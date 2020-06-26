#include "render_device.hpp"

#include <rx/core/string.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <algorithm>

#include <D3D12MemAlloc.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <d3dcompiler.h>
#include <dxgi1_3.h>
#include <rx/core/abort.h>
#include <rx/core/log.h>

#include "adapters/tracy.hpp"
#include "core/constants.hpp"
#include "core/errors.hpp"
#include "rhi/compute_pipeline_state.hpp"
#include "rhi/helpers.hpp"
#include "rhi/render_pipeline_state.hpp"
#include "settings.hpp"
#include "windows/windows_helpers.hpp"

namespace renderer {
    RX_LOG("RenderDevice", logger);

    RX_CONSOLE_BVAR(
        cvar_enable_gpu_based_validation,
        "r.EnableGpuBasedValidation",
        "Enables in-depth validation of operations on the GPU. This has a significant performance cost and should be used sparingly",
        false);

    RX_CONSOLE_IVAR(
        cvar_max_in_flight_gpu_frames, "r.MaxInFlightGpuFrames", "Maximum number of frames that the GPU may work on concurrently", 0, 8, 3);

    RX_CONSOLE_BVAR(cvar_break_on_validation_error,
                    "r.BreakOnValidationError",
                    "Whether to issue a breakpoint when the validation layer encounters an error",
                    false);

    RX_CONSOLE_BVAR(cvar_verify_every_command_list_submission,
                    "r.VerifyEveryCommandListSubmission",
                    "If enabled, SanityEngine will wait for every command list to check for device removed errors",
                    true);

    RenderDevice::RenderDevice(HWND window_handle, // NOLINT(cppcoreguidelines-pro-type-member-init)
                               const glm::uvec2& window_size,
                               const Settings& settings_in)
        : settings{settings_in},
          staging_buffers_to_free{static_cast<Size>(cvar_max_in_flight_gpu_frames->get())},
          scratch_buffers_to_free{static_cast<Size>(cvar_max_in_flight_gpu_frames->get())} {
#ifndef NDEBUG
        // Only enable the debug layer if we're not running in PIX
        const auto result = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&graphics_analysis));
        if(FAILED(result)) {
            enable_debugging();
        }
#endif

        initialize_dxgi();

        create_device_and_swapchain(window_size);

        logger->info("Initialized D3D12 render device");
    }

    RenderDevice::~RenderDevice() { device_context->Flush(); }

    Rx::Ptr<Buffer> RenderDevice::create_buffer(const BufferCreateInfo& create_info) const {
        ZoneScoped;
        auto desc = D3D11_BUFFER_DESC{
            .ByteWidth = create_info.size,
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = 0,
            .MiscFlags = 0,
            .StructureByteStride = 0,
        };

        if(create_info.usage == BufferUsage::StagingBuffer) {
            // Try to get a staging buffer from the pool
        }

        bool should_map = false;

        switch(create_info.usage) {
            case BufferUsage::StagingBuffer:
                desc.BindFlags = 0;
                desc.Usage = D3D11_USAGE_DYNAMIC;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                should_map = true;
                break;

            case BufferUsage::IndexBuffer:
                desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
                break;

            case BufferUsage::VertexBuffer:
                desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                break;

            case BufferUsage::ConstantBuffer:
                desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                desc.Usage = D3D11_USAGE_DYNAMIC;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                should_map = true;
                break;

            case BufferUsage::IndirectCommands:
                [[fallthrough]];
            case BufferUsage::UnorderedAccess:
                desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
                break;

            case BufferUsage::UiVertices:
                desc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER;
                break;
        }

        auto buffer = Rx::make_ptr<Buffer>(RX_SYSTEM_ALLOCATOR);
        const auto result = device->CreateBuffer(&desc, nullptr, &buffer->resource);
        if(FAILED(result)) {
            logger->error("Could not create buffer %s: %s", create_info.name, to_string(result));
            return {};
        }

        set_object_name(buffer->resource.Get(), create_info.name);

        if(should_map) {
            device_context->Map(buffer->resource.Get(), 0, D3D11_MAP_WRITE, 0, &buffer->mapped_data);
        }

        buffer->size = create_info.size;
        buffer->name = create_info.name;

        return buffer;
    }

    Rx::Ptr<Image> RenderDevice::create_image(const ImageCreateInfo& create_info) const {
        auto format = to_dxgi_format(create_info.format); // TODO: Different to_dxgi_format functions for the different kinds of things
        if(format == DXGI_FORMAT_D32_FLOAT) {
            format = DXGI_FORMAT_R32_TYPELESS; // Create depth buffers with a TYPELESS format
        }
        auto desc = D3D11_TEXTURE2D_DESC{
            .Width = static_cast<Uint32>(round(create_info.width)),
            .Height = static_cast<Uint32>(round(create_info.height)),
            .Format = format,
            .Usage = D3D11_USAGE_DEFAULT,
        };

        auto image = Rx::make_ptr<Image>(RX_SYSTEM_ALLOCATOR);
        image->format = create_info.format;

        switch(create_info.usage) {
            case ImageUsage::RenderTarget:
                desc.BindFlags = D3D11_BIND_RENDER_TARGET;
                break;

            case ImageUsage::SampledImage:
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                break;

            case ImageUsage::DepthStencil:
                desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
                break;

            case ImageUsage::UnorderedAccess:
                desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
                break;
        }

        const auto result = device->CreateTexture2D(&desc, nullptr, &image->resource);
        if(FAILED(result)) {
            logger->error("Could not create image %s", create_info.name);
            return {};
        }

        image->name = create_info.name;
        image->width = static_cast<Uint32>(desc.Width);
        image->height = desc.Height;

        set_object_name(image->resource.Get(), create_info.name);

        return image;
    }

    ComPtr<ID3D11RenderTargetView> RenderDevice::create_rtv(const Image& image) const {
        ComPtr<ID3D11RenderTargetView> rtv;

        const auto result = device->CreateRenderTargetView(image.resource.Get(), nullptr, &rtv);
        if(FAILED(result)) {
            logger->error("Could not create RTV for image %s", image.name);
            return {};
        }

        return rtv;
    }

    ComPtr<ID3D11DepthStencilView> RenderDevice::create_dsv(const Image& image) const {
        ComPtr<ID3D11DepthStencilView> dsv;

        const auto result = device->CreateDepthStencilView(image.resource.Get(), nullptr, &dsv);
        if(FAILED(result)) {
            logger->error("Could not create DSV for image %s", image.name);
            return {};
        }

        return dsv;
    }

    ComPtr<ID3D11RenderTargetView> RenderDevice::get_backbuffer_rtv() const {
        ComPtr<ID3D11Texture2D> backbuffer;
        auto result = swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
        if(FAILED(result)) {
            logger->error("Could not get backbuffer image");
            return {};
        }

        ComPtr<ID3D11RenderTargetView> rtv;
        result = device->CreateRenderTargetView(backbuffer.Get(), nullptr, &rtv);
        if(FAILED(result)) {
            logger->error("Could not create RTV for backbuffer image");
            return {};
        }

        return rtv;
    }

    Vec2u RenderDevice::get_backbuffer_size() const {
        DXGI_SWAP_CHAIN_DESC desc;
        swapchain->GetDesc(&desc);

        return {desc.BufferDesc.Width, desc.BufferDesc.Height};
    }

    bool RenderDevice::map_buffer(Buffer& buffer) const {
        const auto result = device_context->Map(buffer.resource.Get(), 0, D3D11_MAP_WRITE, 0, &buffer.mapped_data);
        if(FAILED(result)) {
            logger->error("Could not map buffer: %s", to_string(result));
            return false;
        }

        return true;
    }

    DescriptorType to_descriptor_type(const D3D_SHADER_INPUT_TYPE type) {
        switch(type) {
            case D3D_SIT_CBUFFER:
                return DescriptorType::ConstantBuffer;

            case D3D_SIT_TBUFFER: // NOLINT(bugprone-branch-clone)
                [[fallthrough]];
            case D3D_SIT_TEXTURE:
                [[fallthrough]];
            case D3D_SIT_STRUCTURED:
                return DescriptorType::ShaderResource;

            case D3D_SIT_UAV_RWTYPED:
                [[fallthrough]];
            case D3D_SIT_UAV_RWSTRUCTURED:
                return DescriptorType::UnorderedAccess;

            case D3D_SIT_SAMPLER: // NOLINT(bugprone-branch-clone)
                [[fallthrough]];
            case D3D_SIT_BYTEADDRESS:
                [[fallthrough]];
            case D3D_SIT_UAV_RWBYTEADDRESS:
                [[fallthrough]];
            case D3D_SIT_UAV_APPEND_STRUCTURED:
                [[fallthrough]];
            case D3D_SIT_UAV_CONSUME_STRUCTURED:
                [[fallthrough]];
            case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                [[fallthrough]];
            case D3D_SIT_RTACCELERATIONSTRUCTURE:
                [[fallthrough]];
            case D3D_SIT_UAV_FEEDBACKTEXTURE:
                [[fallthrough]];
            default:
                logger->error("Unknown descriptor type, defaulting to UAV");
                return DescriptorType::UnorderedAccess;
        }
    }

    ComPtr<ID3D11DeviceContext> RenderDevice::get_device_context() const { return device_context; }

    void RenderDevice::begin_frame(const uint64_t frame_count) {
        ZoneScoped;

        wait_for_frame(cur_gpu_frame_idx);
        frame_fence_values[cur_gpu_frame_idx] = frame_count;

        cur_swapchain_idx = swapchain->GetCurrentBackBufferIndex();

        // Don't reset per frame resources on the first frame. This allows the engine to submit work while initializing
        if(!in_init_phase) {
            return_staging_buffers_for_frame(cur_gpu_frame_idx);

            reset_command_allocators_for_frame(cur_gpu_frame_idx);

            destroy_resources_for_frame(cur_gpu_frame_idx);
        }

        transition_swapchain_image_to_render_target();

        in_init_phase = false;
    }

    void RenderDevice::end_frame() {
        ZoneScoped;

        const auto result = swapchain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
        if(result == DXGI_ERROR_DEVICE_HUNG || result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET) {
            logger->error("Device lost on present :(");

            log_dred_report();
        } else {
            cur_swapchain_idx = swapchain->GetCurrentBackBufferIndex();
        }
    }

    void RenderDevice::begin_capture() const {
        if(graphics_analysis) {
            graphics_analysis->BeginCapture();
        }
    }

    void RenderDevice::end_capture() const {
        if(graphics_analysis) {
            graphics_analysis->EndCapture();
        }
    }

    Uint32 RenderDevice::get_max_num_gpu_frames() const { return static_cast<Uint32>(cvar_max_in_flight_gpu_frames->get()); }

    bool RenderDevice::has_separate_device_memory() const { return !is_uma; }

    Buffer RenderDevice::get_staging_buffer(const Uint32 num_bytes) {
        ZoneScoped;

        for(size_t i = 0; i < staging_buffers.size(); i++) {
            if(staging_buffers[i].size >= num_bytes) {
                // Return the first suitable buffer we find
                auto buffer = std::move(staging_buffers[i]);
                staging_buffers.erase(i, i + 1);

                return buffer;
            }
        }

        // No suitable buffer is available, let's make a new one
        return create_staging_buffer(num_bytes);
    }

    ID3D11Device* RenderDevice::get_d3d11_device() const { return device.Get(); }

    void RenderDevice::enable_debugging() {
        auto result = D3D11GetDebugInterface(IID_PPV_ARGS(&debug_controller));
        if(SUCCEEDED(result)) {
            debug_controller->EnableDebugLayer();

            if(cvar_enable_gpu_based_validation->get()) {
                debug_controller->SetEnableGPUBasedValidation(1);
            }

        } else {
            logger->error("Could not enable the D3D12 validation layer: %s", to_string(result).data());
        }

        result = D3D12GetDebugInterface(IID_PPV_ARGS(&dred_settings));
        if(FAILED(result)) {
            logger->error("Could not enable DRED");

        } else {
            dred_settings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            dred_settings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            dred_settings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        }
    }

    void RenderDevice::initialize_dxgi() {
        ZoneScoped;

        ComPtr<IDXGIFactory> basic_factory;
        auto result = CreateDXGIFactory(IID_PPV_ARGS(&basic_factory));
        if(FAILED(result)) {
            Rx::abort("Could not initialize DXGI");
        }

        result = basic_factory->QueryInterface(factory.GetAddressOf());
        if(FAILED(result)) {
            Rx::abort("DXGI is not at a new enough version, please update your graphics drivers");
        }
    }

    void RenderDevice::create_device_and_swapchain(const glm::uvec2& window_size) {
        ZoneScoped;

        const auto swapchain_desc = DXGI_SWAP_CHAIN_DESC{
            .BufferDesc =
                {
                    .Width = static_cast<UINT>(window_size.x),
                    .Height = static_cast<UINT>(window_size.y),
                    .Format = swapchain_format,
                },
            .SampleDesc = {1, 0},

            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = static_cast<Uint32>(cvar_max_in_flight_gpu_frames->get()),

            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING,
        };

        const auto hr = D3D11CreateDeviceAndSwapChain(nullptr,
                                                      D3D_DRIVER_TYPE_HARDWARE,
                                                      nullptr,
                                                      0,
                                                      nullptr,
                                                      0,
                                                      D3D11_SDK_VERSION,
                                                      &swapchain_desc,
                                                      &swapchain,
                                                      &device,
                                                      nullptr,
                                                      &device_context);

        if(FAILED(hr)) {
            const auto msg = Rx::String::format("Could not create device and swapchain: %s", to_string(hr));
            Rx::abort(msg.data());
        }
    }

    void RenderDevice::return_staging_buffers_for_frame(const Uint32 frame_idx) {
        ZoneScoped;
        auto& staging_buffers_for_frame = staging_buffers_to_free[frame_idx];
        staging_buffers += staging_buffers_for_frame;
        staging_buffers_for_frame.clear();
    }

    void RenderDevice::reset_command_allocators_for_frame(const Uint32 frame_idx) {
        ZoneScoped;
        const auto& direct_allocators_for_frame = direct_command_allocators[frame_idx];
        direct_allocators_for_frame.each_pair(
            [](const Uint32& /* thread_id */, const ComPtr<ID3D12CommandAllocator>& allocator) { allocator->Reset(); });

        copy_command_allocators[frame_idx]->Reset();
        compute_command_allocators[frame_idx]->Reset();
    }

    void RenderDevice::destroy_resources_for_frame(const Uint32 frame_idx) {
        ZoneScoped;
        auto& buffers = buffer_deletion_list[frame_idx];
        buffers.each_fwd([&](const Rx::Ptr<Buffer>& buffer) { destroy_resource_immediate(*buffer); });

        buffers.clear();

        auto& images = image_deletion_list[cur_gpu_frame_idx];
        images.each_fwd([&](const Rx::Ptr<Image>& image) { destroy_resource_immediate(*image); });
        images.clear();
    }

    Buffer RenderDevice::create_staging_buffer(const Uint32 num_bytes) {
        const auto desc = CD3DX12_RESOURCE_DESC::Buffer(num_bytes);

        const D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_GENERIC_READ;

        D3D12MA::ALLOCATION_DESC alloc_desc{};
        alloc_desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        auto buffer = Buffer{};
        auto result = device_allocator
                          ->CreateResource(&alloc_desc, &desc, initial_state, nullptr, &buffer.allocation, IID_PPV_ARGS(&buffer.resource));
        if(result == DXGI_ERROR_DEVICE_REMOVED) {
            log_dred_report();

            const auto removed_reason = device->GetDeviceRemovedReason();
            logger->error("Device was removed because: %s", to_string(removed_reason));
        }
        if(FAILED(result)) {
            const auto msg = Rx::String::format("Could not create staging buffer: %s (%u)", to_string(result), result);
            Rx::abort(msg.data());
        }

        buffer.size = num_bytes;
        D3D12_RANGE range{0, num_bytes};
        result = buffer.resource->Map(0, &range, &buffer.mapped_ptr);
        if(FAILED(result)) {
            const auto msg = Rx::String::format("Could not map staging buffer: %s (%u)", to_string(result), result);
            Rx::abort(msg.data());
        }

        const auto msg = Rx::String::format("Staging Buffer %d", staging_buffer_idx);
        set_object_name(buffer.resource.Get(), msg);
        staging_buffer_idx++;

        return buffer;
    }

    Buffer RenderDevice::create_scratch_buffer(const Uint32 num_bytes) {
        constexpr auto alignment = std::max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                                            D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(num_bytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, alignment);

        const auto alloc_desc = D3D12MA::ALLOCATION_DESC{.HeapType = D3D12_HEAP_TYPE_DEFAULT};

        auto scratch_buffer = Buffer{};
        const auto result = device_allocator->CreateResource(&alloc_desc,
                                                             &desc,
                                                             D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                             nullptr,
                                                             &scratch_buffer.allocation,
                                                             IID_PPV_ARGS(&scratch_buffer.resource));
        if(FAILED(result)) {
            logger->error("Could not create scratch buffer: %s", to_string(result));
        }

        scratch_buffer.size = num_bytes;
        set_object_name(scratch_buffer.resource.Get(), Rx::String::format("Scratch buffer %d", scratch_buffer_counter));
        scratch_buffer_counter++;

        return scratch_buffer;
    }

    void RenderDevice::log_dred_report() const {
        ComPtr<ID3D12DeviceRemovedExtendedData1> dred;
        auto result = device->QueryInterface(IID_PPV_ARGS(&dred));
        if(FAILED(result)) {
            logger->error("Could not retrieve DRED report");
            return;
        }

        D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 breadcrumbs;
        result = dred->GetAutoBreadcrumbsOutput1(&breadcrumbs);
        if(FAILED(result)) {
            return;
        }

        D3D12_DRED_PAGE_FAULT_OUTPUT1 page_faults;
        result = dred->GetPageFaultAllocationOutput1(&page_faults);
        if(FAILED(result)) {
            return;
        }

        logger->error("Command history:\n%s", breadcrumb_output_to_string(breadcrumbs));
        logger->error(page_fault_output_to_string(page_faults).data());
    }

    Rx::Ptr<RenderDevice> make_render_device(GLFWwindow* window, const Settings& settings) {
        auto hwnd = glfwGetWin32Window(window);

        glm::ivec2 framebuffer_size{};
        glfwGetFramebufferSize(window, &framebuffer_size.x, &framebuffer_size.y);

        logger->info("Creating D3D12 backend");

        return Rx::make_ptr<RenderDevice>(RX_SYSTEM_ALLOCATOR, hwnd, framebuffer_size, settings);
    }
} // namespace renderer
