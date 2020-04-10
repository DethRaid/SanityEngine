#include "d3d12_render_device.hpp"

#include <minitrace.h>
#include <rx/console/interface.h>
#include <rx/console/variable.h>
#include <rx/core/abort.h>
#include <rx/core/log.h>

#include "../../core/constants.hpp"
#include "../../core/cvar_names.hpp"
#include "helpers.hpp"

RX_CONSOLE_BVAR(r_enable_debug_layers,
                "r.EnableDebugLayers",
                "Enable the D3D12 debug layers, allowing one to better debug new D3D12 code",
                true);

RX_LOG("D3D12RenderDevice", logger);

namespace render {
    D3D12RenderDevice::D3D12RenderDevice(rx::memory::allocator& allocator, const HWND window_handle, const rx::math::vec2i& window_size)
        : internal_allocator{&allocator} {
        if(*r_enable_debug_layers) {
            enable_validation_layer();
        }

        initialize_dxgi();

        select_adapter();

        create_queues();

        auto* num_frames_ref = rx::console::interface::find_variable_by_name(NUM_IN_FLIGHT_FRAMES_NAME);
        if(num_frames_ref == nullptr) {
            const auto msg = rx::string::format("Can not find console variable %s", NUM_IN_FLIGHT_FRAMES_NAME);
            rx::abort(msg.data());
        }

        create_swapchain(window_handle, window_size, num_frames_ref->cast<UINT>()->get());

        create_command_allocators();

        create_standard_root_signature();

        create_descriptor_heaps();

        initialize_swapchain_descriptors();

        initialize_dma();

        initialize_standard_resource_binding_mappings();

        create_shader_compiler();

        create_material_resource_binder();
    }

    D3D12RenderDevice::~D3D12RenderDevice() {
        allocator->Release();
    }

    void D3D12RenderDevice::enable_validation_layer() {
        const auto res = D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller));
        if(SUCCEEDED(res)) {
            debug_controller->EnableDebugLayer();

        } else {
            logger->error("Could not enable the D3D12 validation layer");
        }
    }

    void D3D12RenderDevice::initialize_dxgi() {
        MTR_SCOPE("D3D12RenderDevice", "initialize_dxgi");

        ComPtr<IDXGIFactory> basic_factory;
        auto result = CreateDXGIFactory(IID_PPV_ARGS(&basic_factory));
        if(FAILED(result)) {
            rx::abort("Could not initialize DXGI");
        }

        result = basic_factory->QueryInterface(factory.GetAddressOf());
        if(FAILED(result)) {
            rx::abort("DXGI is not at a new enough version, please update your graphics drivers");
        }
    }

    void D3D12RenderDevice::select_adapter() {
        MTR_SCOPE("D3D12RenderDevice", "select_adapter");

        // We want an adapter:
        // - Not integrated, if possible

        // TODO: Figure out how to get the number of adapters in advance
        rx::vector<ComPtr<IDXGIAdapter>> adapters{*internal_allocator};
        adapters.reserve(5);

        {
            UINT adapter_idx = 0;
            ComPtr<IDXGIAdapter> cur_adapter;
            while(factory->EnumAdapters(adapter_idx, &cur_adapter) != DXGI_ERROR_NOT_FOUND) {
                adapters.push_back(cur_adapter);
                adapter_idx++;
            }
        }

        // TODO: Score adapters based on things like supported feature level and available vram

        adapters.each_fwd([&](const ComPtr<IDXGIAdapter>& cur_adapter) {
            DXGI_ADAPTER_DESC desc;
            cur_adapter->GetDesc(&desc);

            if(desc.VendorId == INTEL_PCI_VENDOR_ID && adapters.size() > 1) {
                // Prefer something other then the Intel GPU
                return RX_ITERATION_CONTINUE;
            }

            ComPtr<ID3D12Device> try_device;
            auto res = D3D12CreateDevice(cur_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&try_device));
            if(SUCCEEDED(res)) {
                // check the features we care about
                D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12_options;
                try_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12_options, sizeof(d3d12_options));
                if(d3d12_options.ResourceBindingTier != D3D12_RESOURCE_BINDING_TIER_3) {
                    // Resource binding tier three means we can have partially bound descriptor array. Nova relies on partially bound
                    // descriptor arrays, so we need it
                    // Thus - if we find an adapter without full descriptor indexing support, we ignore it

                    return RX_ITERATION_CONTINUE;
                }

                adapter = cur_adapter;

                device = try_device;

                device->QueryInterface(device1.GetAddressOf());

                // Save information about the device
                D3D12_FEATURE_DATA_ARCHITECTURE arch;
                res = device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch, sizeof(D3D12_FEATURE_DATA_ARCHITECTURE));
                if(SUCCEEDED(res)) {
                    is_uma = arch.CacheCoherentUMA;
                }

                D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5;
                res = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
                if(SUCCEEDED(res)) {
                    render_pass_tier = options5.RenderPassesTier;
                    has_raytracing = options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
                }

                if(*r_enable_debug_layers) {
                    res = device->QueryInterface(info_queue.GetAddressOf());
                    if(SUCCEEDED(res)) {
                        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
                        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                    }
                }

                return RX_ITERATION_STOP;
            }

            return RX_ITERATION_CONTINUE;
        });

        if(!device) {
            rx::abort("Could not find a suitable D3D12 adapter");
        }

        set_object_name(device.Get(), "D3D12 Device");
    }

    void D3D12RenderDevice::create_queues() {
        MTR_SCOPE("D3D12RenderDevice", "create_queues");

        // One graphics queue and one optional DMA queue
        D3D12_COMMAND_QUEUE_DESC graphics_queue_desc{};
        graphics_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        auto result = device->CreateCommandQueue(&graphics_queue_desc, IID_PPV_ARGS(&direct_command_queue));
        if(FAILED(result)) {
            rx::abort("Could not create graphics command queue");
        }

        set_object_name(direct_command_queue.Get(), "Direct Queue");

        // TODO: Add an async compute queue, when the time comes

        if(!is_uma) {
            // No need to care about DMA on UMA cause we can just map everything
            D3D12_COMMAND_QUEUE_DESC dma_queue_desc{};
            dma_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
            result = device->CreateCommandQueue(&dma_queue_desc, IID_PPV_ARGS(&async_copy_queue));
            if(FAILED(result)) {
                logger->warning("Could not create a DMA queue on a non-UMA adapter, data transfers will have to use the graphics queue");

            } else {
                set_object_name(async_copy_queue.Get(), "DMA queue");
            }
        }
    }

    void D3D12RenderDevice::create_swapchain(HWND window_handle, const rx::math::vec2i& window_size, const UINT num_images) {
        DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
        swapchain_desc.Width = window_size.x;
        swapchain_desc.Height = window_size.y;
        swapchain_desc.Format = swapchain_format;

        swapchain_desc.SampleDesc = {1};
        swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchain_desc.BufferCount = num_images;

        swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        ComPtr<IDXGISwapChain1> swapchain1;
        auto hr = factory->CreateSwapChainForHwnd(direct_command_queue.Get(),
                                                  window_handle,
                                                  &swapchain_desc,
                                                  nullptr,
                                                  nullptr,
                                                  swapchain1.GetAddressOf());
        if(FAILED(hr)) {
            const auto msg = rx::string::format("Could not create swapchain: %u", hr);
            rx::abort(msg.data());
        }

        hr = swapchain1->QueryInterface(swapchain.GetAddressOf());
        if(FAILED(hr)) {
            rx::abort("Could not get new swapchain interface, please update your drivers");
        }
    }

    void D3D12RenderDevice::create_command_allocators() {
        MTR_SCOPE("D3D12RenderDevice", "create_command_allocators");

        auto result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&direct_command_queue));
        if(FAILED(result)) {
            rx::abort("Could not create direct command allocator");
        }

        result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&async_copy_queue));
        if(FAILED(result)) {
            rx::abort("Could not create copy command allocator");
        }
    }

    void D3D12RenderDevice::create_descriptor_heaps() {
        const auto& [new_cbv_srv_uav_heap, new_cbv_srv_uav_size] = create_descriptor_allocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                                                               65536);
        cbv_srv_uav_heap = new_cbv_srv_uav_heap;
        cbv_srv_uav_size = new_cbv_srv_uav_size;

        const auto& [new_rtv_heap, new_rtv_size] = create_descriptor_allocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1024);
        rtv_heap = new_rtv_heap;
        rtv_size = new_rtv_size;

        const auto& [new_dsv_heap, new_dsv_size] = create_descriptor_allocator(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 32);
        dsv_heap = new_dsv_heap;
        dsv_size = new_dsv_size;
    }

    rx::pair<ComPtr<ID3D12DescriptorHeap>, UINT> D3D12RenderDevice::create_descriptor_allocator(
        const D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type, const uint32_t num_descriptors) const {
        ComPtr<ID3D12DescriptorHeap> heap;

        D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
        heap_desc.Type = descriptor_type;
        heap_desc.NumDescriptors = num_descriptors;
        heap_desc.Flags = (descriptor_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE :
                                                                                       D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
        device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap));
        const auto descriptor_size = device->GetDescriptorHandleIncrementSize(descriptor_type);

        return {heap, descriptor_size};
    }

    static void* dma_allocate(size_t size, size_t /* alignment */, void* user_data) {
        auto* allocator = static_cast<rx::memory::allocator*>(user_data);
        return allocator->allocate(size);
    }

    static void dma_free(void* memory, void* user_data) {
        auto* allocator = static_cast<rx::memory::allocator*>(user_data);
        allocator->deallocate(memory);
    }

    void D3D12RenderDevice::initialize_dma() {
        D3D12MA::ALLOCATOR_DESC allocator_desc{};
        allocator_desc.pDevice = device.Get();
        allocator_desc.pAdapter = adapter.Get();

        D3D12MA::ALLOCATION_CALLBACKS allocation_callbacks{};
        allocation_callbacks.pAllocate = dma_allocate;
        allocation_callbacks.pFree = dma_free;
        allocation_callbacks.pUserData = internal_allocator;

        allocator_desc.pAllocationCallbacks = &allocation_callbacks;

        const auto result = D3D12MA::CreateAllocator(&allocator_desc, &allocator);
        if(FAILED(result)) {
            rx::abort("Could not initialize DMA");
        }
    }

    void D3D12RenderDevice::create_shader_compiler() {
        
    }
} // namespace render
