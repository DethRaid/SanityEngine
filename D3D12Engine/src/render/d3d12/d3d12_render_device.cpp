#include "d3d12_render_device.hpp"

#include <minitrace.h>
#include <rx/console/interface.h>
#include <rx/console/variable.h>
#include <rx/core/abort.h>
#include <rx/core/log.h>
#include <rx/core/math/round.h>

#include "../../core/constants.hpp"
#include "../../core/cvar_names.hpp"
#include "d3d12_compute_command_list.hpp"
#include "d3d12_framebuffer.hpp"
#include "d3d12_render_command_list.hpp"
#include "d3d12_resource_command_list.hpp"
#include "d3dx12.hpp"
#include "helpers.hpp"
#include "resources.hpp"

RX_CONSOLE_BVAR(r_enable_debug_layers,
                "r.EnableDebugLayers",
                "Enable the D3D12 debug layers, allowing one to better debug new D3D12 code",
                true);

RX_LOG("D3D12RenderDevice", logger);

#ifdef interface
#undef interface
#endif

using rx::utility::move;

namespace render {
    D3D12RenderDevice::D3D12RenderDevice(rx::memory::allocator& allocator, const HWND window_handle, const rx::math::vec2i& window_size)
        : internal_allocator{&allocator}, rtv_cache{allocator}, command_list_done_fences{allocator} {
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

        create_descriptor_heaps();

        // initialize_swapchain_descriptors();

        initialize_dma();

        create_shader_compiler();

        // initialize_standard_resource_binding_mappings();

        create_standard_root_signature();

        create_material_resource_binder();
    }

    D3D12RenderDevice::~D3D12RenderDevice() { device_allocator->Release(); }

    rx::ptr<Buffer> D3D12RenderDevice::create_buffer(rx::memory::allocator& allocator, const BufferCreateInfo& create_info) {
        MTR_SCOPE("D3D12RenderDevice", "create_buffer");

        const auto desc = CD3DX12_RESOURCE_DESC::Buffer(create_info.size);

        D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON;

        D3D12MA::ALLOCATION_DESC alloc_desc{};
        switch(create_info.usage) {
            case BufferUsage::StagingBuffer:
                [[fallthrough]];
            case BufferUsage::ConstantBuffer:
                alloc_desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
                initial_state = D3D12_RESOURCE_STATE_GENERIC_READ;
                break;

            case BufferUsage::IndirectCommands:
                [[fallthrough]];
            case BufferUsage::UnorderedAccess:
                [[fallthrough]];
            case BufferUsage::IndexBuffer:
                [[fallthrough]];
            case BufferUsage::VertexBuffer:
                alloc_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
                initial_state = D3D12_RESOURCE_STATE_COMMON;
                break;
        }

        auto buffer = rx::make_ptr<D3D12Buffer>(allocator);
        const auto result = device_allocator->CreateResource(&alloc_desc,
                                                             &desc,
                                                             initial_state,
                                                             nullptr,
                                                             &buffer->allocation,
                                                             IID_PPV_ARGS(&buffer->resource));
        if(FAILED(result)) {
            logger->error("Could not create buffer %s", create_info.name);
            return {};
        }

        buffer->size = create_info.size;

        set_object_name(*buffer->resource.Get(), create_info.name);

        return move(buffer);
    }

    rx::ptr<Image> D3D12RenderDevice::create_image(rx::memory::allocator& allocator, const ImageCreateInfo& create_info) {
        MTR_SCOPE("D3D12RenderDevice", "create_image");

        const auto format = to_dxgi_format(create_info.format);
        const auto desc = CD3DX12_RESOURCE_DESC::Tex2D(format, rx::math::round(create_info.width), rx::math::round(create_info.height));

        D3D12MA::ALLOCATION_DESC alloc_desc{};
        alloc_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        auto image = rx::make_ptr<D3D12Image>(allocator);
        image->format = format;

        const auto initial_state = [&] {
            switch(create_info.usage) {
                case ImageUsage::RenderTarget:
                    return D3D12_RESOURCE_STATE_RENDER_TARGET;

                case ImageUsage::SampledImage:
                    return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

                case ImageUsage::DepthStencil:
                    return D3D12_RESOURCE_STATE_DEPTH_WRITE;

                case ImageUsage::UnorderedAccess:
                    return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            }

            logger->warning("Unrecognized usage for image %s, defaulting to the common resource state", create_info.name);
            return D3D12_RESOURCE_STATE_COMMON;
        }();

        const auto result = device_allocator->CreateResource(&alloc_desc,
                                                             &desc,
                                                             initial_state,
                                                             nullptr,
                                                             &image->allocation,
                                                             IID_PPV_ARGS(&image->resource));
        if(FAILED(result)) {
            logger->error("Could not create image %s", create_info.name);
            return {};
        }

        set_object_name(*image->resource.Get(), create_info.name);

        return image;
    }

    rx::ptr<Framebuffer> D3D12RenderDevice::create_framebuffer(const rx::vector<const Image*>& render_targets, const Image* depth_target) {
        MTR_SCOPE("D3D12RenderDevice", "create_framebuffer");

        auto framebuffer = rx::make_ptr<D3D12Framebuffer>(*internal_allocator);

        framebuffer->rtv_handles.reserve(render_targets.size());
        render_targets.each_fwd([&](const Image* image) {
            const auto* d3d12_image = static_cast<const D3D12Image*>(image);

            D3D12_RENDER_TARGET_VIEW_DESC desc{};
            desc.Format = d3d12_image->format;
            desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            desc.Texture2D.PlaneSlice = 0;
            desc.Texture2D.MipSlice = 0;

            const auto handle = rtv_allocator->get_next_free_descriptor();

            device->CreateRenderTargetView(d3d12_image->resource.Get(), &desc, handle);

            framebuffer->rtv_handles.push_back(handle);
        });

        if(depth_target != nullptr) {
            const auto* d3d12_depth_target = static_cast<const D3D12Image*>(depth_target);

            D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
            desc.Format = d3d12_depth_target->format;
            desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipSlice = 0;

            const auto handle = dsv_allocator->get_next_free_descriptor();

            device->CreateDepthStencilView(d3d12_depth_target->resource.Get(), &desc, handle);

            framebuffer->dsv_handle = handle;
        }

        return framebuffer;
    }

    void* D3D12RenderDevice::map_buffer(const Buffer& buffer) {
        const auto& d3d12_buffer = static_cast<const D3D12Buffer&>(buffer);
        MTR_SCOPE("D3D12RenderEngine", "map_buffer");

        void* ptr;
        D3D12_RANGE range{0, d3d12_buffer.size};
        const auto result = d3d12_buffer.resource->Map(0, &range, &ptr);
        if(FAILED(result)) {
            logger->error("Could not map buffer");
            return nullptr;
        }

        return ptr;
    }

    void D3D12RenderDevice::destroy_buffer(rx::ptr<Buffer> /* buffer */) {
        // We don't need to do anything special, D3D12 has destructors
    }

    void D3D12RenderDevice::destroy_image(rx::ptr<Image> /* image */) {
        // Again nothing to do, D3D12 still has destructors
    }

    void D3D12RenderDevice::destroy_framebuffer(rx::ptr<Framebuffer> framebuffer) {
        auto* d3d12_framebuffer = static_cast<D3D12Framebuffer*>(framebuffer.get());

        d3d12_framebuffer->rtv_handles.each_fwd(
            [&](const D3D12_CPU_DESCRIPTOR_HANDLE handle) { rtv_allocator->return_descriptor(handle); });

        if(d3d12_framebuffer->dsv_handle) {
            dsv_allocator->return_descriptor(*d3d12_framebuffer->dsv_handle);
        }
    }

    rx::ptr<ComputePipelineState> D3D12RenderDevice::create_compute_pipeline_state() {}

    rx::ptr<ResourceCommandList> D3D12RenderDevice::create_resource_command_list() {
        MTR_SCOPE("D3D12RenderDevice", "get_resoruce_command_list");

        ComPtr<ID3D12GraphicsCommandList> commands;
        ComPtr<ID3D12CommandList> cmds;
        const auto result = device->CreateCommandList(0,
                                                      D3D12_COMMAND_LIST_TYPE_COPY,
                                                      copy_command_allocator.Get(),
                                                      nullptr,
                                                      IID_PPV_ARGS(cmds.GetAddressOf()));
        if(FAILED(result)) {
            logger->error("Could not create resource command list");
            return {};
        }

        cmds->QueryInterface(commands.GetAddressOf());

        return rx::make_ptr<D3D12ResourceCommandList>(*internal_allocator, *internal_allocator, commands, *this);
    }

    rx::ptr<ComputeCommandList> D3D12RenderDevice::create_compute_command_list() {
        MTR_SCOPE("D3D12RenderDevice", "get_compute_command_list");

        ComPtr<ID3D12GraphicsCommandList> commands;
        ComPtr<ID3D12CommandList> cmds;
        const auto result = device->CreateCommandList(0,
                                                      D3D12_COMMAND_LIST_TYPE_COMPUTE,
                                                      compute_command_allocator.Get(),
                                                      nullptr,
                                                      IID_PPV_ARGS(cmds.GetAddressOf()));
        if(FAILED(result)) {
            logger->error("Could not create compute command list");
            return {};
        }

        cmds->QueryInterface(commands.GetAddressOf());

        return rx::make_ptr<D3D12ComputeCommandList>(*internal_allocator, *internal_allocator, commands, *this);
    }

    rx::ptr<RenderCommandList> D3D12RenderDevice::create_render_command_list() {
        MTR_SCOPE("D3D12RenderDevice", "create_graphics_command_list");

        ComPtr<ID3D12GraphicsCommandList> commands;
        ComPtr<ID3D12CommandList> cmds;
        const auto result = device->CreateCommandList(0,
                                                      D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                      direct_command_allocator.Get(),
                                                      nullptr,
                                                      IID_PPV_ARGS(cmds.GetAddressOf()));
        if(FAILED(result)) {
            logger->error("Could not create render command list");
            return {};
        }

        cmds->QueryInterface(commands.GetAddressOf());

        return rx::make_ptr<D3D12RenderCommandList>(*internal_allocator, *internal_allocator, commands, *this);
    }

    void D3D12RenderDevice::submit_command_list(rx::ptr<CommandList> commands) {
        rx::ptr<D3D12CommandList> d3d12_commands = move(commands);

        auto* d3d12_command_list = d3d12_commands->get_command_list();

        // First implementation - run everything on the same queue, because it's easy
        // Eventually I'll come up with a fancy way to use multiple queues

        direct_command_queue->ExecuteCommandLists(1, &d3d12_command_list);

        auto command_list_done_fence = get_next_command_list_done_fence();

        direct_command_queue->Signal(command_list_done_fence.Get(), CPU_FENCE_SIGNALED);
    }

    bool D3D12RenderDevice::has_separate_device_memory() const { return !is_uma; }

    ComPtr<ID3D12Fence> D3D12RenderDevice::get_next_command_list_done_fence() {
        if(!command_list_done_fences.is_empty()) {
            auto fence = command_list_done_fences.last();
            command_list_done_fences.pop_back();

            return fence;
        }

        ComPtr<ID3D12Fence> fence;
        device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()));

        return fence;
    }

    auto D3D12RenderDevice::get_shader_resource_descriptor_size() const { return cbv_srv_uav_size; }

    auto* D3D12RenderDevice::get_d3d12_device() const { return device.Get(); }

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

        set_object_name(*device.Get(), "D3D12 Device");
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

        set_object_name(*direct_command_queue.Get(), "Direct Queue");

        // TODO: Add an async compute queue, when the time comes

        if(!is_uma) {
            // No need to care about DMA on UMA cause we can just map everything
            D3D12_COMMAND_QUEUE_DESC dma_queue_desc{};
            dma_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
            result = device->CreateCommandQueue(&dma_queue_desc, IID_PPV_ARGS(&async_copy_queue));
            if(FAILED(result)) {
                logger->warning("Could not create a DMA queue on a non-UMA adapter, data transfers will have to use the graphics queue");

            } else {
                set_object_name(*async_copy_queue.Get(), "DMA queue");
            }
        }
    }

    void D3D12RenderDevice::create_swapchain(HWND window_handle, const rx::math::vec2i& window_size, const UINT num_images) {
        MTR_SCOPE("D3D12RenderDevice", "create_swapchain");
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

        auto result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&direct_command_allocator));
        if(FAILED(result)) {
            rx::abort("Could not create direct command allocator");
        }

        result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(compute_command_allocator.GetAddressOf()));
        if(FAILED(result)) {
            rx::abort("Could not create compute command allocator");
        }

        result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&copy_command_allocator));
        if(FAILED(result)) {
            rx::abort("Could not create copy command allocator");
        }
    }

    void D3D12RenderDevice::create_descriptor_heaps() {
        MTR_SCOPE("D3D12RenderDevice", "create_descriptor_heaps");
        const auto& [new_cbv_srv_uav_heap, new_cbv_srv_uav_size] = create_descriptor_allocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                                                               65536);
        cbv_srv_uav_heap = new_cbv_srv_uav_heap;
        cbv_srv_uav_size = new_cbv_srv_uav_size;

        const auto& [rtv_heap, rtv_size] = create_descriptor_allocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1024);
        rtv_allocator = rx::make_ptr<D3D12DescriptorAllocator>(*internal_allocator, *internal_allocator, rtv_heap, rtv_size);

        const auto& [dsv_heap, dsv_size] = create_descriptor_allocator(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 32);
        dsv_allocator = rx::make_ptr<D3D12DescriptorAllocator>(*internal_allocator, *internal_allocator, dsv_heap, dsv_size);
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

    static void* dma_allocate(const size_t size, size_t /* alignment */, void* user_data) {
        auto* allocator = static_cast<rx::memory::allocator*>(user_data);
        return allocator->allocate(size);
    }

    static void dma_free(void* memory, void* user_data) {
        auto* allocator = static_cast<rx::memory::allocator*>(user_data);
        allocator->deallocate(memory);
    }

    void D3D12RenderDevice::initialize_dma() {
        MTR_SCOPE("D3D12RenderDevice", "iniitialize_dma");

        D3D12MA::ALLOCATOR_DESC allocator_desc{};
        allocator_desc.pDevice = device.Get();
        allocator_desc.pAdapter = adapter.Get();

        D3D12MA::ALLOCATION_CALLBACKS allocation_callbacks{};
        allocation_callbacks.pAllocate = dma_allocate;
        allocation_callbacks.pFree = dma_free;
        allocation_callbacks.pUserData = internal_allocator;

        allocator_desc.pAllocationCallbacks = &allocation_callbacks;

        const auto result = D3D12MA::CreateAllocator(&allocator_desc, &device_allocator);
        if(FAILED(result)) {
            rx::abort("Could not initialize DMA");
        }
    }

    void D3D12RenderDevice::create_shader_compiler() {
        MTR_SCOPE("D3D12RenderDevice", "create_shader_compiler");

        auto hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(dxc_library.GetAddressOf()));
        if(FAILED(hr)) {
            rx::abort("Could not create DXC Library instance");
        }

        hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(dxc_compiler.GetAddressOf()));
        if(FAILED(hr)) {
            rx::abort("Could not create DXC instance");
        }
    }

    void D3D12RenderDevice::create_standard_root_signature() {
        MTR_SCOPE("D3D12RenderDevice", "create_standard_root_signature");

        rx::vector<CD3DX12_ROOT_PARAMETER> root_parameters{*internal_allocator, 4};

        // Root constants for material index and camera index
        root_parameters[0].InitAsConstants(2, 0);

        // Camera data buffer
        root_parameters[1].InitAsShaderResourceView(0);

        // Material data buffer
        root_parameters[2].InitAsShaderResourceView(1);

        // Textures array
        rx::vector<D3D12_DESCRIPTOR_RANGE> descriptor_table_ranges{*internal_allocator};
        D3D12_DESCRIPTOR_RANGE textures_array;
        textures_array.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        textures_array.NumDescriptors = MAX_NUM_TEXTURES;
        textures_array.BaseShaderRegister = 3;
        textures_array.RegisterSpace = 0;
        textures_array.OffsetInDescriptorsFromTableStart = 0;
        descriptor_table_ranges.push_back(move(textures_array));

        root_parameters[3].InitAsDescriptorTable(static_cast<UINT>(descriptor_table_ranges.size()), descriptor_table_ranges.data());

        rx::vector<D3D12_STATIC_SAMPLER_DESC> static_samplers{*internal_allocator, 3};

        // Point sampler
        auto& point_sampler_desc = static_samplers[0];
        point_sampler_desc.Filter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
        point_sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        point_sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        point_sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        point_sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

        auto& linear_sampler = static_samplers[1];
        linear_sampler.Filter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
        linear_sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        linear_sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        linear_sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        linear_sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        linear_sampler.RegisterSpace = 1;

        auto& trilinear_sampler = static_samplers[2];
        trilinear_sampler.Filter = D3D12_FILTER_ANISOTROPIC;
        trilinear_sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        trilinear_sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        trilinear_sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        trilinear_sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        trilinear_sampler.MaxAnisotropy = 8;
        trilinear_sampler.RegisterSpace = 2;

        D3D12_ROOT_SIGNATURE_DESC root_signature_desc{};
        root_signature_desc.NumParameters = static_cast<UINT>(root_parameters.size());
        root_signature_desc.pParameters = root_parameters.data();
        root_signature_desc.NumStaticSamplers = static_cast<UINT>(static_samplers.size());
        root_signature_desc.pStaticSamplers = static_samplers.data();

        standard_root_signature = compile_root_signature(root_signature_desc);
        if(!standard_root_signature) {
            rx::abort("Could not create standard root signature");
        }

        set_object_name(*standard_root_signature.Get(), "Standard Root Signature");
    }

    ComPtr<ID3D12RootSignature> D3D12RenderDevice::compile_root_signature(const D3D12_ROOT_SIGNATURE_DESC& root_signature_desc) const {
        MTR_SCOPE("D3D12RenderDevice", "compile_root_signature");

        ComPtr<ID3DBlob> root_signature_blob;
        ComPtr<ID3DBlob> error_blob;
        auto result = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_signature_blob, &error_blob);
        if(FAILED(result)) {
            const rx::string msg{*internal_allocator, reinterpret_cast<char*>(error_blob->GetBufferPointer()), error_blob->GetBufferSize()};
            logger->error("Could not create root signature: %s", msg);
            return {};
        }

        ComPtr<ID3D12RootSignature> sig;
        result = device->CreateRootSignature(0,
                                             root_signature_blob->GetBufferPointer(),
                                             root_signature_blob->GetBufferSize(),
                                             IID_PPV_ARGS(&sig));
        if(FAILED(result)) {
            logger->error("Could not create root signature");
            return {};
        }

        return sig;
    }

    void D3D12RenderDevice::create_material_resource_binder() {}
} // namespace render
