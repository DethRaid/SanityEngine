#include "render_device.hpp"

#include <DirectXMath.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <D3D12MemAlloc.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <d3dcompiler.h>
#include <dxgi1_3.h>
#include <minitrace.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "../core/ensure.hpp"
#include "../settings.hpp"
#include "../windows/windows_helpers.hpp"
#include "d3dx12.hpp"
#include "../core/errors.hpp"

namespace rhi {
    RenderDevice::RenderDevice(const HWND window_handle,
                               const glm::uvec2& window_size,
                               const Settings& settings_in) // NOLINT(cppcoreguidelines-pro-type-member-init)
        : settings{settings_in},
          logger{spdlog::stdout_color_st("RenderDevice")},
          command_lists_by_frame{settings.num_in_flight_gpu_frames},
          buffer_deletion_list{settings.num_in_flight_gpu_frames},
          image_deletion_list{settings.num_in_flight_gpu_frames},
          staging_buffers_to_free{settings.num_in_flight_gpu_frames},
          scratch_buffers_to_free{settings.num_in_flight_gpu_frames} {
#ifndef NDEBUG
        // Only enable the debug layer if we're not running in PIX
        const auto result = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&graphics_analysis));
        if(FAILED(result)) {
            enable_debugging();
        }

        logger->set_level(spdlog::level::debug);
#endif

        initialize_dxgi();

        select_adapter();

        create_queues();

        create_swapchain(window_handle, window_size, settings.num_in_flight_gpu_frames);

        create_gpu_frame_synchronization_objects();

        create_command_allocators();

        create_descriptor_heaps();

        initialize_swapchain_descriptors();

        initialize_dma();

        create_static_sampler_descriptions();

        create_standard_root_signature();

        create_material_resource_binders();

        create_standard_graphics_pipeline_input_layout();

        logger->info("Initialized D3D12 render device");
    }

    RenderDevice::~RenderDevice() {
        for(uint32_t i = 0; i < settings.num_in_flight_gpu_frames; i++) {
            wait_for_frame(i);
            direct_command_queue->Wait(frame_fences.Get(), frame_fence_values[i]);
        }

        wait_gpu_idle(0);

        for(auto& buffer : staging_buffers) {
            buffer.allocation->Release();
        }

        device_allocator->Release();
    }

    std::unique_ptr<Buffer> RenderDevice::create_buffer(const BufferCreateInfo& create_info) {
        MTR_SCOPE("RenderDevice", "create_buffer");

        auto desc = CD3DX12_RESOURCE_DESC::Buffer(create_info.size);

        D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON;
        bool should_map = false;

        D3D12MA::ALLOCATION_DESC alloc_desc{};
        switch(create_info.usage) {
            case BufferUsage::StagingBuffer:
                [[fallthrough]];
            case BufferUsage::ConstantBuffer:
                alloc_desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
                initial_state = D3D12_RESOURCE_STATE_GENERIC_READ;
                should_map = true;
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

            case BufferUsage::RaytracingAccelerationStructure:
                alloc_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
                initial_state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
                desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                break;

            default:
                logger->warn("Unknown buffer usage {}", create_info.usage);
        }

        auto buffer = std::make_unique<Buffer>();
        const auto result = device_allocator->CreateResource(&alloc_desc,
                                                             &desc,
                                                             initial_state,
                                                             nullptr,
                                                             &buffer->allocation,
                                                             IID_PPV_ARGS(&buffer->resource));
        if(FAILED(result)) {
            logger->error("Could not create buffer {}: {}", create_info.name, to_string(result));
            return {};
        }

        if(should_map) {
            D3D12_RANGE mapped_range{0, create_info.size};
            buffer->resource->Map(0, &mapped_range, &buffer->mapped_ptr);
        }

        buffer->size = create_info.size;

        buffer->name = create_info.name;

        set_object_name(*buffer->resource.Get(), create_info.name);

        return move(buffer);
    }

    std::unique_ptr<Image> RenderDevice::create_image(const ImageCreateInfo& create_info) {
        MTR_SCOPE("RenderDevice", "create_image");

        const auto format = to_dxgi_format(create_info.format);
        auto desc = CD3DX12_RESOURCE_DESC::Tex2D(format,
                                                 static_cast<uint32_t>(round(create_info.width)),
                                                 static_cast<uint32_t>(round(create_info.height)));

        D3D12MA::ALLOCATION_DESC alloc_desc{};
        alloc_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        auto image = std::make_unique<D3D12Image>();
        image->format = format;

        D3D12_RESOURCE_STATES initial_state;
        switch(create_info.usage) {
            case ImageUsage::RenderTarget:
                initial_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
                desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
                break;

            case ImageUsage::SampledImage:
                initial_state = D3D12_RESOURCE_STATE_COPY_DEST;
                break;

            case ImageUsage::DepthStencil:
                initial_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
                desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
                break;

            case ImageUsage::UnorderedAccess:
                initial_state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                break;
        }

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

        image->name = create_info.name;
        image->width = static_cast<uint32_t>(desc.Width);
        image->height = desc.Height;

        set_object_name(*image->resource.Get(), create_info.name);

        return image;
    }

    std::unique_ptr<Framebuffer> RenderDevice::create_framebuffer(const std::vector<const Image*>& render_targets,
                                                                  const Image* depth_target) {
        MTR_SCOPE("RenderDevice", "create_framebuffer");

        auto framebuffer = std::make_unique<D3D12Framebuffer>();

        float width = 0;
        float height = 0;

        framebuffer->rtv_handles.reserve(render_targets.size());
        uint32_t i{0};
        for(const auto* image : render_targets) {
            const auto* d3d12_image = static_cast<const D3D12Image*>(image);

            if(width != 0 && width != d3d12_image->width) {
                logger->error(
                    "Render target {} has width {}, which is different from the width {} of the previous render target. All render targets must have the same width",
                    i,
                    d3d12_image->width,
                    width);
            }

            width = static_cast<float>(d3d12_image->width);

            if(height != 0 && height != d3d12_image->height) {
                logger->error(
                    "Render target {} has height {}, which is different from the height {} of the previous render target. All render targets must have the same height",
                    i,
                    d3d12_image->height,
                    height);
            }

            height = static_cast<float>(d3d12_image->height);

            const auto handle = rtv_allocator->get_next_free_descriptor();

            device->CreateRenderTargetView(d3d12_image->resource.Get(), nullptr, handle);

            framebuffer->rtv_handles.push_back(handle);

            i++;
        }

        if(depth_target != nullptr) {
            const auto* d3d12_depth_target = static_cast<const D3D12Image*>(depth_target);

            if(width != 0 && width != d3d12_depth_target->width) {
                logger->error(
                    "Depth target has width {}, which is different from the width {} of the render targets. The depth target must have the same width as the render targets",
                    i,
                    d3d12_depth_target->width,
                    width);
            }

            width = static_cast<float>(d3d12_depth_target->width);

            if(height != 0 && height != d3d12_depth_target->height) {
                logger->error(
                    "Depth target has height {}, which is different from the height {} of the render targets. The depth target must have the same height as the render targets",
                    i,
                    d3d12_depth_target->height,
                    height);
            }

            height = static_cast<float>(d3d12_depth_target->height);

            D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
            desc.Format = d3d12_depth_target->format;
            desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipSlice = 0;

            const auto handle = dsv_allocator->get_next_free_descriptor();

            device->CreateDepthStencilView(d3d12_depth_target->resource.Get(), &desc, handle);

            framebuffer->dsv_handle = handle;
        }

        framebuffer->width = width;
        framebuffer->height = height;

        return framebuffer;
    }

    Framebuffer* RenderDevice::get_backbuffer_framebuffer() {
        const auto cur_swapchain_index = swapchain->GetCurrentBackBufferIndex();

        ENSURE(cur_swapchain_index < swapchain_framebuffers.size(),
               "Not enough swapchain framebuffers for current swapchain index {}",
               cur_swapchain_index);

        return &swapchain_framebuffers[cur_swapchain_index];
    }

    void* RenderDevice::map_buffer(const Buffer& buffer) {
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

    void RenderDevice::destroy_buffer(std::unique_ptr<Buffer> buffer) {
        buffer_deletion_list[cur_gpu_frame_idx].emplace_back(static_cast<D3D12Buffer*>(buffer.release()));
    }

    void RenderDevice::destroy_image(std::unique_ptr<Image> image) {
        image_deletion_list[cur_gpu_frame_idx].emplace_back(static_cast<D3D12Image*>(image.release()));
    }

    void RenderDevice::destroy_framebuffer(const std::unique_ptr<Framebuffer> framebuffer) {
        auto* d3d12_framebuffer = static_cast<D3D12Framebuffer*>(framebuffer.get());

        for(const D3D12_CPU_DESCRIPTOR_HANDLE handle : d3d12_framebuffer->rtv_handles) {
            rtv_allocator->return_descriptor(handle);
        }

        if(d3d12_framebuffer->dsv_handle) {
            dsv_allocator->return_descriptor(*d3d12_framebuffer->dsv_handle);
        }
    }

    std::unique_ptr<ComputePipelineState> RenderDevice::create_compute_pipeline_state(const std::vector<uint8_t>& compute_shader) {
        auto compute_pipeline = std::make_unique<D3D12ComputePipelineState>();

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
        desc.CS.BytecodeLength = compute_shader.size();
        desc.CS.pShaderBytecode = compute_shader.data();

        device->CreateComputePipelineState(&desc, IID_PPV_ARGS(compute_pipeline->pso.GetAddressOf()));

        return compute_pipeline;
    }

    std::unique_ptr<RenderPipelineState> RenderDevice::create_render_pipeline_state(const RenderPipelineStateCreateInfo& create_info) {
        return create_pipeline_state(create_info, *standard_root_signature.Get());
    }

    DescriptorType to_descriptor_type(const D3D_SHADER_INPUT_TYPE type) {
        switch(type) {
            case D3D_SIT_CBUFFER:
                return DescriptorType::ConstantBuffer;

            case D3D_SIT_TBUFFER:
                [[fallthrough]];
            case D3D_SIT_TEXTURE:
                [[fallthrough]];
            case D3D_SIT_STRUCTURED:
                return DescriptorType::ShaderResource;

            case D3D_SIT_UAV_RWTYPED:
                [[fallthrough]];
            case D3D_SIT_UAV_RWSTRUCTURED:
                return DescriptorType::UnorderedAccess;

            case D3D_SIT_SAMPLER:
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
                spdlog::error("Unknown descriptor type, defaulting to UAV");
                return DescriptorType::UnorderedAccess;
        }
    }

    void RenderDevice::destroy_compute_pipeline_state(std::unique_ptr<ComputePipelineState> /* pipeline_state */) {
        // Nothing to explicitly do, the destructors will take care of us
    }

    void RenderDevice::destroy_render_pipeline_state(std::unique_ptr<RenderPipelineState> /* pipeline_state */) {
        // Nothing to do, destructors will take care of it
    }

    std::unique_ptr<ResourceCommandList> RenderDevice::create_resource_command_list() {
        MTR_SCOPE("RenderDevice", "get_resoruce_command_list");

        ComPtr<ID3D12GraphicsCommandList4> commands;
        ComPtr<ID3D12CommandList> cmds;
        const auto result = device->CreateCommandList(0,
                                                      D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                      direct_command_allocators[cur_gpu_frame_idx].Get(),
                                                      nullptr,
                                                      IID_PPV_ARGS(cmds.GetAddressOf()));
        if(FAILED(result)) {
            logger->error("Could not create resource command list");
            return {};
        }

        cmds->QueryInterface(commands.GetAddressOf());

        return std::make_unique<D3D12ResourceCommandList>(commands, *this);
    }

    std::unique_ptr<ComputeCommandList> RenderDevice::create_compute_command_list() {
        MTR_SCOPE("RenderDevice", "get_compute_command_list");

        ComPtr<ID3D12GraphicsCommandList4> commands;
        ComPtr<ID3D12CommandList> cmds;
        const auto result = device->CreateCommandList(0,
                                                      D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                      direct_command_allocators[cur_gpu_frame_idx].Get(),
                                                      nullptr,
                                                      IID_PPV_ARGS(cmds.GetAddressOf()));
        if(FAILED(result)) {
            logger->error("Could not create compute command list");
            return {};
        }

        cmds->QueryInterface(commands.GetAddressOf());

        return std::make_unique<D3D12ComputeCommandList>(commands, *this);
    }

    std::unique_ptr<RenderCommandList> RenderDevice::create_render_command_list() {
        MTR_SCOPE("RenderDevice", "create_graphics_command_list");

        ComPtr<ID3D12GraphicsCommandList4> commands;
        ComPtr<ID3D12CommandList> cmds;
        const auto result = device->CreateCommandList(0,
                                                      D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                      direct_command_allocators[cur_gpu_frame_idx].Get(),
                                                      nullptr,
                                                      IID_PPV_ARGS(cmds.GetAddressOf()));
        if(FAILED(result)) {
            logger->error("Could not create render command list");
            return {};
        }

        // logger->debug("Creating a render command list with command allocator {}",
        // fmt::ptr(direct_command_allocators[cur_gpu_frame_idx].Get()));

        cmds->QueryInterface(commands.GetAddressOf());

        return std::make_unique<D3D12RenderCommandList>(commands, *this);
    }

    void RenderDevice::submit_command_list(std::unique_ptr<CommandList> commands) {
        auto* d3d12_commands = dynamic_cast<D3D12CommandList*>(commands.release());

        d3d12_commands->prepare_for_submission();

        command_lists_by_frame[cur_gpu_frame_idx].push_back(std::unique_ptr<D3D12CommandList>{d3d12_commands});
    }

    BindGroupBuilder& RenderDevice::get_material_bind_group_builder_for_frame(const uint32_t frame_idx) {
        ENSURE(frame_idx < material_bind_group_builder.size(), "Not enough material resource binders for every swapchain image");

        return material_bind_group_builder[frame_idx];
    }

    void RenderDevice::begin_frame(const uint64_t frame_count) {
        MTR_SCOPE("RenderDevice", "begin_frame");

        cur_gpu_frame_idx = (cur_gpu_frame_idx + 1) % settings.num_in_flight_gpu_frames;

        wait_for_frame(cur_gpu_frame_idx);
        frame_fence_values[cur_gpu_frame_idx] = frame_count;

        cur_swapchain_idx = swapchain->GetCurrentBackBufferIndex();

        return_staging_buffers_for_frame(cur_gpu_frame_idx);

        reset_command_allocators_for_frame(cur_gpu_frame_idx);

        destroy_resources_for_frame(cur_gpu_frame_idx);

        transition_swapchain_image_to_render_target();
    }

    void RenderDevice::end_frame() {
        MTR_SCOPE("RenderDevice", "end_frame");
        // Transition the swapchain image into the correct format and request presentation
        transition_swapchain_image_to_presentable();

        flush_batched_command_lists();

        direct_command_queue->Signal(frame_fences.Get(), frame_fence_values[cur_gpu_frame_idx]);

        {
            MTR_SCOPE("RenderDevice", "present");
            const auto result = swapchain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
            if(result == DXGI_ERROR_DEVICE_HUNG || result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET) {
                logger->error("Device lost on present :(");

                if(settings.enable_gpu_crash_reporting) {
                    retrieve_dred_report();
                }
            }
        }
    }

    uint32_t RenderDevice::get_cur_gpu_frame_idx() { return cur_gpu_frame_idx; }

    void RenderDevice::begin_capture() {
        if(graphics_analysis) {
            graphics_analysis->BeginCapture();
        }
    }

    void RenderDevice::end_capture() {
        if(graphics_analysis) {
            graphics_analysis->EndCapture();
        }
    }

    bool RenderDevice::has_separate_device_memory() const { return !is_uma; }

    D3D12StagingBuffer RenderDevice::get_staging_buffer(const uint32_t num_bytes) {
        size_t best_fit_idx = staging_buffers.size();
        for(size_t i = 0; i < staging_buffers.size(); i++) {
            if(staging_buffers[i].size >= num_bytes) {
                if(best_fit_idx >= staging_buffers.size() || staging_buffers[i].size < staging_buffers[best_fit_idx].size) {
                    // The current buffer is more suitable than the previous best buffer
                    best_fit_idx = i;
                }
            }
        }

        if(best_fit_idx < staging_buffers.size()) {
            // We found a valid staging buffer!
            auto buffer = std::move(staging_buffers[best_fit_idx]);
            staging_buffers.erase(staging_buffers.begin() + best_fit_idx);

            return std::move(buffer);

        } else {
            // No suitable buffer is available, let's make a new one
            return create_staging_buffer(num_bytes);
        }
    }

    void RenderDevice::return_staging_buffer(D3D12StagingBuffer&& buffer) {
        staging_buffers_to_free[cur_gpu_frame_idx].push_back(std::move(buffer));
    }

    D3D12Buffer RenderDevice::get_scratch_buffer(const uint32_t num_bytes) {
        size_t best_fit_idx = scratch_buffers.size();
        for(size_t i = 0; i < scratch_buffers.size(); i++) {
            if(scratch_buffers[i].size >= num_bytes) {
                if(best_fit_idx >= scratch_buffers.size() || scratch_buffers[i].size < scratch_buffers[best_fit_idx].size) {
                    // The current buffer is a better fit than the previous best fit buffer
                    best_fit_idx = i;
                }
            }
        }

        if(best_fit_idx < scratch_buffers.size()) {
            // We already have a suitable scratch buffer!
            auto buffer = std::move(scratch_buffers[best_fit_idx]);
            scratch_buffers.erase(scratch_buffers.begin() + best_fit_idx);

            return buffer;

        } else {
            return create_scratch_buffer(num_bytes);
        }
    }

    void RenderDevice::return_scratch_buffer(D3D12Buffer&& buffer) { scratch_buffers_to_free[cur_gpu_frame_idx].push_back(buffer); }

    UINT RenderDevice::get_shader_resource_descriptor_size() const { return cbv_srv_uav_size; }

    ID3D12Device* RenderDevice::get_d3d12_device() const { return device.Get(); }

    void RenderDevice::enable_debugging() {
        auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller));
        if(SUCCEEDED(result)) {
            debug_controller->EnableDebugLayer();

        } else {
            logger->error("Could not enable the D3D12 validation layer: {}", to_string(result));
        }

        if(settings.enable_gpu_crash_reporting) {
            result = D3D12GetDebugInterface(IID_PPV_ARGS(&dred_settings));
            if(FAILED(result)) {
                logger->error("Could not enable DRED");

            } else {
                dred_settings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
                dred_settings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);

                ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> dred1_settings;
                dred_settings->QueryInterface(dred1_settings.GetAddressOf());
                if(dred1_settings) {
                    dred1_settings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
                }
            }
        }
    }

    void RenderDevice::initialize_dxgi() {
        MTR_SCOPE("RenderDevice", "initialize_dxgi");

        ComPtr<IDXGIFactory> basic_factory;
        auto result = CreateDXGIFactory(IID_PPV_ARGS(&basic_factory));
        if(FAILED(result)) {
            critical_error("Could not initialize DXGI");
        }

        result = basic_factory->QueryInterface(factory.GetAddressOf());
        if(FAILED(result)) {
            critical_error("DXGI is not at a new enough version, please update your graphics drivers");
        }
    }

    void RenderDevice::select_adapter() {
        MTR_SCOPE("RenderDevice", "select_adapter");

        // We want an adapter:
        // - Not integrated, if possible

        // TODO: Figure out how to get the number of adapters in advance
        std::vector<ComPtr<IDXGIAdapter>> adapters;
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

        for(const ComPtr<IDXGIAdapter>& cur_adapter : adapters) {
            DXGI_ADAPTER_DESC desc;
            cur_adapter->GetDesc(&desc);

            if(desc.VendorId == INTEL_PCI_VENDOR_ID && adapters.size() > 1) {
                // Prefer something other then the Intel GPU
                continue;
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

                    logger->warn("Ignoring adapter {} - Doesn't have the flexible resource binding that Sanity Engine needs",
                                 from_wide_string(desc.Description));

                    continue;
                }

                D3D12_FEATURE_DATA_SHADER_MODEL shader_model{D3D_SHADER_MODEL_6_5};
                res = try_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shader_model, sizeof(shader_model));
                if(FAILED(res)) {
                    logger->warn("Ignoring adapter {} - Could not check the supported shader model: {}",
                                 from_wide_string(desc.Description),
                                 to_string(res));

                    continue;

                } else if(shader_model.HighestShaderModel < D3D_SHADER_MODEL_6_5) {
                    // Only supports old-ass shaders

                    logger->warn("Ignoring adapter {} - Doesn't support the shader model Sanity Engine uses",
                                 from_wide_string(desc.Description));

                    // continue;
                }

                adapter = cur_adapter;

                device = try_device;

                device->QueryInterface(device1.GetAddressOf());
                device->QueryInterface(device5.GetAddressOf());

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

#ifndef NDEBUG
                if(!settings.enable_gpu_crash_reporting) {
                    res = device->QueryInterface(info_queue.GetAddressOf());
                    if(SUCCEEDED(res)) {
                        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
                        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                    }
                }
#endif

                break;

            } else {
                logger->warn("Ignoring adapter {} - doesn't support D3D12", from_wide_string(desc.Description));
            }

            continue;
        }

        if(!device) {
            critical_error("Could not find a suitable D3D12 adapter");
        }

        set_object_name(*device.Get(), "D3D12 Device");
    }

    void RenderDevice::create_queues() {
        MTR_SCOPE("RenderDevice", "create_queues");

        // One graphics queue and one optional DMA queue
        D3D12_COMMAND_QUEUE_DESC graphics_queue_desc{};
        graphics_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        graphics_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        graphics_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        auto result = device->CreateCommandQueue(&graphics_queue_desc, IID_PPV_ARGS(&direct_command_queue));
        if(FAILED(result)) {
            critical_error("Could not create graphics command queue");
        }

        set_object_name(*direct_command_queue.Get(), "Direct Queue");

        // TODO: Add an async compute queue, when the time comes

        if(!is_uma) {
            // No need to care about DMA on UMA cause we can just map everything
            D3D12_COMMAND_QUEUE_DESC dma_queue_desc{};
            dma_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
            dma_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            dma_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

            result = device->CreateCommandQueue(&dma_queue_desc, IID_PPV_ARGS(&async_copy_queue));
            if(FAILED(result)) {
                logger->warn("Could not create a DMA queue on a non-UMA adapter, data transfers will have to use the graphics queue");

            } else {
                set_object_name(*async_copy_queue.Get(), "DMA queue");
            }
        }
    }

    void RenderDevice::create_swapchain(HWND window_handle, const XMINT2& window_size, const UINT num_images) {
        MTR_SCOPE("RenderDevice", "create_swapchain");
        DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
        swapchain_desc.Width = static_cast<UINT>(window_size.x);
        swapchain_desc.Height = static_cast<UINT>(window_size.y);
        swapchain_desc.Format = swapchain_format;

        swapchain_desc.SampleDesc = {1, 0};
        swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchain_desc.BufferCount = num_images;

        swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        ComPtr<IDXGISwapChain1> swapchain1;
        auto hr = factory->CreateSwapChainForHwnd(direct_command_queue.Get(),
                                                  window_handle,
                                                  &swapchain_desc,
                                                  nullptr,
                                                  nullptr,
                                                  swapchain1.GetAddressOf());
        if(FAILED(hr)) {
            const auto msg = fmt::format("Could not create swapchain: {}", to_string(hr));
            critical_error(msg.data());
        }

        hr = swapchain1->QueryInterface(swapchain.GetAddressOf());
        if(FAILED(hr)) {
            critical_error("Could not get new swapchain interface, please update your drivers");
        }
    }

    void RenderDevice::create_gpu_frame_synchronization_objects() {
        frame_fence_values.resize(settings.num_in_flight_gpu_frames);

        device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frame_fences));
        set_object_name(*frame_fences.Get(), fmt::format("Frame Synchronization Fence"));

        frame_event = CreateEvent(nullptr, false, false, nullptr);
    }

    void RenderDevice::create_command_allocators() {
        MTR_SCOPE("RenderDevice", "create_command_allocators");

        direct_command_allocators.resize(settings.num_in_flight_gpu_frames);
        compute_command_allocators.resize(settings.num_in_flight_gpu_frames);
        copy_command_allocators.resize(settings.num_in_flight_gpu_frames);

        for(uint32_t i = 0; i < settings.num_in_flight_gpu_frames; i++) {
            HRESULT result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                            IID_PPV_ARGS(direct_command_allocators[i].GetAddressOf()));
            if(FAILED(result)) {
                critical_error(fmt::format("Could not create direct command allocator for frame {}", i));
            }

            set_object_name(*direct_command_allocators[i].Get(), fmt::format("Direct Command Allocator {}", i));

            result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
                                                    IID_PPV_ARGS(compute_command_allocators[i].GetAddressOf()));
            if(FAILED(result)) {
                critical_error(fmt::format("Could not create compute command allocator for frame {}", i));
            }

            set_object_name(*compute_command_allocators[i].Get(), fmt::format("Compute Command Allocator {}", i));

            result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(copy_command_allocators[i].GetAddressOf()));
            if(FAILED(result)) {
                critical_error(fmt::format("Could not create copy command allocator for frame {}", i));
            }

            set_object_name(*copy_command_allocators[i].Get(), fmt::format("Copy Command Allocator {}", i));
        }
    }

    void RenderDevice::create_descriptor_heaps() {
        MTR_SCOPE("RenderDevice", "create_descriptor_heaps");

        const auto [new_cbv_srv_uav_heap,
                    new_cbv_srv_uav_size] = create_descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                                   MAX_NUM_TEXTURES * 2 * settings.num_in_flight_gpu_frames);

        cbv_srv_uav_heap = new_cbv_srv_uav_heap;
        cbv_srv_uav_size = new_cbv_srv_uav_size;

        const auto [rtv_heap, rtv_size] = create_descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1024);
        rtv_allocator = std::make_unique<D3D12DescriptorAllocator>(rtv_heap, rtv_size);

        const auto [dsv_heap, dsv_size] = create_descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 32);
        dsv_allocator = std::make_unique<D3D12DescriptorAllocator>(dsv_heap, dsv_size);
    }

    void RenderDevice::initialize_swapchain_descriptors() {
        DXGI_SWAP_CHAIN_DESC1 desc;
        swapchain->GetDesc1(&desc);
        swapchain_images.resize(desc.BufferCount);
        swapchain_framebuffers.reserve(desc.BufferCount);

        for(uint32_t i = 0; i < desc.BufferCount; i++) {
            swapchain->GetBuffer(i, IID_PPV_ARGS(&swapchain_images[i]));

            const auto rtv_handle = rtv_allocator->get_next_free_descriptor();

            device->CreateRenderTargetView(swapchain_images[i].Get(), nullptr, rtv_handle);

            D3D12Framebuffer framebuffer;
            framebuffer.rtv_handles.push_back(rtv_handle);
            framebuffer.width = static_cast<float>(desc.Width);
            framebuffer.height = static_cast<float>(desc.Height);

            swapchain_framebuffers.push_back(std::move(framebuffer));

            set_object_name(*swapchain_images[i].Get(), fmt::format("Swapchain image {}", i));
        }
    }

    std::pair<ID3D12DescriptorHeap*, UINT> RenderDevice::create_descriptor_heap(const D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type,
                                                                                const uint32_t num_descriptors) const {
        ID3D12DescriptorHeap* heap;

        D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
        heap_desc.Type = descriptor_type;
        heap_desc.NumDescriptors = num_descriptors;
        heap_desc.Flags = (descriptor_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE :
                                                                                       D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

        const auto result = device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap));
        if(FAILED(result)) {
            logger->error("Could not create descriptor heap: {}", to_string(result));

            return {{}, 0};
        }

        const auto descriptor_size = device->GetDescriptorHandleIncrementSize(descriptor_type);

        return {heap, descriptor_size};
    }

    void RenderDevice::initialize_dma() {
        MTR_SCOPE("RenderDevice", "iniitialize_dma");

        D3D12MA::ALLOCATOR_DESC allocator_desc{};
        allocator_desc.pDevice = device.Get();
        allocator_desc.pAdapter = adapter.Get();

        const auto result = D3D12MA::CreateAllocator(&allocator_desc, &device_allocator);
        if(FAILED(result)) {
            critical_error("Could not initialize DMA");
        }
    }

    void RenderDevice::create_static_sampler_descriptions() {
        point_sampler_desc = D3D12_STATIC_SAMPLER_DESC{.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
                                                       .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                       .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                       .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                       .MipLODBias = 0,
                                                       .MaxAnisotropy = 0,
                                                       .ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS,
                                                       .MinLOD = 0,
                                                       .MaxLOD = D3D12_FLOAT32_MAX,
                                                       .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL};

        linear_sampler_desc = D3D12_STATIC_SAMPLER_DESC{.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
                                                        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                        .MipLODBias = 0,
                                                        .MaxAnisotropy = 0,
                                                        .ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS,
                                                        .MinLOD = 0,
                                                        .MaxLOD = D3D12_FLOAT32_MAX,
                                                        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL};
    }

    void RenderDevice::create_standard_root_signature() {
        MTR_SCOPE("RenderDevice", "create_standard_root_signature");

        std::vector<CD3DX12_ROOT_PARAMETER> root_parameters{6};

        // Root constants for material index and camera index
        root_parameters[0].InitAsConstants(2, 0);

        // Camera data buffer
        root_parameters[1].InitAsShaderResourceView(0);

        // Material data buffer
        root_parameters[2].InitAsShaderResourceView(1);

        // Lights buffer
        root_parameters[3].InitAsShaderResourceView(2);

        // Raytracing acceleration structure
        root_parameters[4].InitAsShaderResourceView(3);

        // Textures array
        std::vector<D3D12_DESCRIPTOR_RANGE> descriptor_table_ranges;
        D3D12_DESCRIPTOR_RANGE textures_array;
        textures_array.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        textures_array.NumDescriptors = MAX_NUM_TEXTURES;
        textures_array.BaseShaderRegister = 16;
        textures_array.RegisterSpace = 0;
        textures_array.OffsetInDescriptorsFromTableStart = 0;
        descriptor_table_ranges.push_back(move(textures_array));

        root_parameters[5].InitAsDescriptorTable(static_cast<UINT>(descriptor_table_ranges.size()), descriptor_table_ranges.data());

        std::vector<D3D12_STATIC_SAMPLER_DESC> static_samplers{3};

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

        D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
        root_signature_desc.NumParameters = static_cast<UINT>(root_parameters.size());
        root_signature_desc.pParameters = root_parameters.data();
        root_signature_desc.NumStaticSamplers = static_cast<UINT>(static_samplers.size());
        root_signature_desc.pStaticSamplers = static_samplers.data();
        root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        standard_root_signature = compile_root_signature(root_signature_desc);
        if(!standard_root_signature) {
            critical_error("Could not create standard root signature");
        }

        set_object_name(*standard_root_signature.Get(), "Standard Root Signature");
    }

    ComPtr<ID3D12RootSignature> RenderDevice::compile_root_signature(const D3D12_ROOT_SIGNATURE_DESC& root_signature_desc) const {
        MTR_SCOPE("RenderDevice", "compile_root_signature");

        ComPtr<ID3DBlob> root_signature_blob;
        ComPtr<ID3DBlob> error_blob;
        auto result = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_signature_blob, &error_blob);
        if(FAILED(result)) {
            const std::string msg{reinterpret_cast<char*>(error_blob->GetBufferPointer()), error_blob->GetBufferSize()};
            logger->error("Could not create root signature: {}", msg);
            return {};
        }

        ComPtr<ID3D12RootSignature> sig;
        result = device->CreateRootSignature(0,
                                             root_signature_blob->GetBufferPointer(),
                                             root_signature_blob->GetBufferSize(),
                                             IID_PPV_ARGS(&sig));
        if(FAILED(result)) {
            logger->error("Could not create root signature: {}", to_string(result));
            return {};
        }

        return sig;
    }

    void RenderDevice::create_material_resource_binders() {
        std::unordered_map<std::string, RootDescriptorDescription> root_descriptors;
        root_descriptors.emplace("cameras", RootDescriptorDescription{1, DescriptorType::ShaderResource});
        root_descriptors.emplace("material_buffer", RootDescriptorDescription{2, DescriptorType::ShaderResource});
        root_descriptors.emplace("lights", RootDescriptorDescription{3, DescriptorType::ShaderResource});
        root_descriptors.emplace("raytracing_scene", RootDescriptorDescription{4, DescriptorType::ShaderResource});

        material_bind_group_builder.reserve(settings.num_in_flight_gpu_frames);

        CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle{cbv_srv_uav_heap->GetCPUDescriptorHandleForHeapStart(),
                                                 static_cast<INT>(next_free_cbv_srv_uav_descriptor),
                                                 cbv_srv_uav_size};
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle{cbv_srv_uav_heap->GetGPUDescriptorHandleForHeapStart(),
                                                 static_cast<INT>(next_free_cbv_srv_uav_descriptor),
                                                 cbv_srv_uav_size};

        for(uint32_t i = 0; i < settings.num_in_flight_gpu_frames; i++) {
            std::unordered_map<std::string, DescriptorTableDescriptorDescription> descriptor_tables;
            // Textures array _always_ is at the start of the descriptor heap
            descriptor_tables.emplace("textures", DescriptorTableDescriptorDescription{DescriptorType::ShaderResource, cpu_handle});

            std::unordered_map<uint32_t, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_gpu_handles;
            descriptor_table_gpu_handles.emplace(static_cast<uint32_t>(root_descriptors.size() + 1), gpu_handle);

            material_bind_group_builder.emplace_back(*device.Get(),
                                                     *cbv_srv_uav_heap.Get(),
                                                     cbv_srv_uav_size,
                                                     root_descriptors,
                                                     descriptor_tables,
                                                     descriptor_table_gpu_handles);

            cpu_handle.Offset(MAX_NUM_TEXTURES, cbv_srv_uav_size);
            gpu_handle.Offset(MAX_NUM_TEXTURES, cbv_srv_uav_size);

            next_free_cbv_srv_uav_descriptor += MAX_NUM_TEXTURES;
        }
    }

    void RenderDevice::create_standard_graphics_pipeline_input_layout() {
        standard_graphics_pipeline_input_layout.reserve(4);

        standard_graphics_pipeline_input_layout.push_back(
            D3D12_INPUT_ELEMENT_DESC{.SemanticName = "Position",
                                     .SemanticIndex = 0,
                                     .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                                     .InputSlot = 0,
                                     .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
                                     .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                     .InstanceDataStepRate = 0});

        standard_graphics_pipeline_input_layout.push_back(
            D3D12_INPUT_ELEMENT_DESC{.SemanticName = "Normal",
                                     .SemanticIndex = 0,
                                     .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                                     .InputSlot = 0,
                                     .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
                                     .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                     .InstanceDataStepRate = 0});

        standard_graphics_pipeline_input_layout.push_back(
            D3D12_INPUT_ELEMENT_DESC{.SemanticName = "Color",
                                     .SemanticIndex = 0,
                                     .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                     .InputSlot = 0,
                                     .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
                                     .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                     .InstanceDataStepRate = 0});

        standard_graphics_pipeline_input_layout.push_back(
            D3D12_INPUT_ELEMENT_DESC{.SemanticName = "Texcoord",
                                     .SemanticIndex = 0,
                                     .Format = DXGI_FORMAT_R32G32_FLOAT,
                                     .InputSlot = 0,
                                     .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
                                     .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                     .InstanceDataStepRate = 0});
    }

    std::vector<D3D12_SHADER_INPUT_BIND_DESC> RenderDevice::get_bindings_from_shader(const std::vector<uint8_t>& shader) const {
        ComPtr<ID3D12ShaderReflection> reflection;
        auto result = D3DReflect(shader.data(), shader.size() * sizeof(uint8_t), IID_PPV_ARGS(&reflection));
        if(FAILED(result)) {
            logger->error("Could not retrieve shader reflection information: {}", to_string(result));
        }

        D3D12_SHADER_DESC desc;
        result = reflection->GetDesc(&desc);
        if(FAILED(result)) {
            logger->error("Could not get shader description");
        }

        std::vector<D3D12_SHADER_INPUT_BIND_DESC> input_descs(desc.BoundResources);

        for(uint32_t i = 0; i < desc.BoundResources; i++) {
            result = reflection->GetResourceBindingDesc(i, &input_descs[i]);
            if(FAILED(result)) {
                logger->error("Could not get binding information for resource idx {}", i);
            }
        }

        return input_descs;
    }

    std::unique_ptr<RenderPipelineState> RenderDevice::create_pipeline_state(const RenderPipelineStateCreateInfo& create_info,
                                                                             ID3D12RootSignature& root_signature) {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};

        desc.pRootSignature = &root_signature;

        desc.VS.BytecodeLength = create_info.vertex_shader.size();
        desc.VS.pShaderBytecode = create_info.vertex_shader.data();

        if(create_info.pixel_shader) {
            desc.PS.BytecodeLength = create_info.pixel_shader->size();
            desc.PS.pShaderBytecode = create_info.pixel_shader->data();
        }

        desc.InputLayout.NumElements = static_cast<UINT>(standard_graphics_pipeline_input_layout.size());
        desc.InputLayout.pInputElementDescs = standard_graphics_pipeline_input_layout.data();
        desc.PrimitiveTopologyType = to_d3d12_primitive_topology_type(create_info.primitive_type);

        // Rasterizer state
        {
            auto& output_rasterizer_state = desc.RasterizerState;
            const auto& rasterizer_state = create_info.rasterizer_state;

            output_rasterizer_state.FillMode = to_d3d12_fill_mode(rasterizer_state.fill_mode);
            output_rasterizer_state.CullMode = to_d3d12_cull_mode(rasterizer_state.cull_mode);
            output_rasterizer_state.FrontCounterClockwise = rasterizer_state.front_face_counter_clockwise ? 1 : 0;
            output_rasterizer_state.DepthBias = static_cast<UINT>(
                rasterizer_state.depth_bias); // TODO: Figure out what the actual fuck D3D12 depth bias is
            output_rasterizer_state.DepthBiasClamp = rasterizer_state.max_depth_bias;
            output_rasterizer_state.SlopeScaledDepthBias = rasterizer_state.slope_scaled_depth_bias;
            output_rasterizer_state.MultisampleEnable = rasterizer_state.num_msaa_samples > 1 ? 1 : 0;
            output_rasterizer_state.AntialiasedLineEnable = rasterizer_state.enable_line_antialiasing;
            output_rasterizer_state.ConservativeRaster = rasterizer_state.enable_conservative_rasterization ?
                                                             D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON :
                                                             D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

            desc.SampleMask = UINT_MAX;
            desc.SampleDesc.Count = rasterizer_state.num_msaa_samples;
        }

        // Depth stencil state
        {
            auto& output_ds_state = desc.DepthStencilState;
            const auto& ds_state = create_info.depth_stencil_state;

            output_ds_state.DepthEnable = ds_state.enable_depth_test ? 1 : 0;
            output_ds_state.DepthWriteMask = ds_state.enable_depth_write ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
            output_ds_state.DepthFunc = to_d3d12_comparison_func(ds_state.depth_func);

            output_ds_state.StencilEnable = ds_state.enable_stencil_test ? 1 : 0;
            output_ds_state.StencilReadMask = ds_state.stencil_read_mask;
            output_ds_state.StencilWriteMask = ds_state.stencil_write_mask;
            output_ds_state.FrontFace.StencilFailOp = to_d3d12_stencil_op(ds_state.front_face.fail_op);
            output_ds_state.FrontFace.StencilDepthFailOp = to_d3d12_stencil_op(ds_state.front_face.depth_fail_op);
            output_ds_state.FrontFace.StencilPassOp = to_d3d12_stencil_op(ds_state.front_face.pass_op);
            output_ds_state.FrontFace.StencilFunc = to_d3d12_comparison_func(ds_state.front_face.compare_op);
            output_ds_state.BackFace.StencilFailOp = to_d3d12_stencil_op(ds_state.back_face.fail_op);
            output_ds_state.BackFace.StencilDepthFailOp = to_d3d12_stencil_op(ds_state.back_face.depth_fail_op);
            output_ds_state.BackFace.StencilPassOp = to_d3d12_stencil_op(ds_state.back_face.pass_op);
            output_ds_state.BackFace.StencilFunc = to_d3d12_comparison_func(ds_state.back_face.compare_op);
        }

        // Blend state
        {
            const auto& blend_state = create_info.blend_state;
            desc.BlendState.AlphaToCoverageEnable = blend_state.enable_alpha_to_coverage ? 1 : 0;
            for(uint32_t i = 0; i < blend_state.render_target_blends.size(); i++) {
                auto& output_rt_blend = desc.BlendState.RenderTarget[i];
                const auto& rt_blend = blend_state.render_target_blends[i];

                output_rt_blend.BlendEnable = rt_blend.enabled ? 1 : 0;
                output_rt_blend.SrcBlend = to_d3d12_blend(rt_blend.source_color_blend_factor);
                output_rt_blend.DestBlend = to_d3d12_blend(rt_blend.destination_color_blend_factor);
                output_rt_blend.BlendOp = to_d3d12_blend_op(rt_blend.color_blend_op);
                output_rt_blend.SrcBlendAlpha = to_d3d12_blend(rt_blend.source_alpha_blend_factor);
                output_rt_blend.DestBlendAlpha = to_d3d12_blend(rt_blend.destination_alpha_blend_factor);
                output_rt_blend.BlendOpAlpha = to_d3d12_blend_op(rt_blend.alpha_blend_op);
                output_rt_blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            }
        }

        ENSURE(create_info.render_target_formats.size() + (create_info.depth_stencil_format ? 1 : 0) > 0,
               "Must have at least one render target or depth target");
        ENSURE(create_info.render_target_formats.size() < 8,
               "May not have more than 8 render targets - you have {}",
               create_info.render_target_formats.size());

        desc.NumRenderTargets = static_cast<UINT>(create_info.render_target_formats.size());
        for(uint32_t i = 0; i < create_info.render_target_formats.size(); i++) {
            desc.RTVFormats[i] = to_dxgi_format(create_info.render_target_formats[i]);
        }
        if(create_info.depth_stencil_format) {
            desc.DSVFormat = to_dxgi_format(*create_info.depth_stencil_format);
        } else {
            desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
        }

        auto pipeline = std::make_unique<D3D12RenderPipelineState>();
        pipeline->root_signature = &root_signature;

        const auto result = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(pipeline->pso.GetAddressOf()));
        if(FAILED(result)) {
            logger->error("Could not create render pipeline {}: {}", create_info.name, to_string(result));
            return {};
        }

        set_object_name(*pipeline->pso.Get(), create_info.name);

        return pipeline;
    }

    void RenderDevice::flush_batched_command_lists() {
        // Submit all the command lists we batched up
        auto& lists = command_lists_by_frame[cur_gpu_frame_idx];
        for(auto& commands : lists) {
            auto* d3d12_command_list = static_cast<ID3D12CommandList*>(commands->get_command_list());

            // First implementation - run everything on the same queue, because it's easy
            // Eventually I'll come up with a fancy way to use multiple queues

            // TODO: Actually figure out how to use multiple queues
            direct_command_queue->ExecuteCommandLists(1, &d3d12_command_list);

            if(settings.enable_gpu_crash_reporting) {
                auto command_list_done_fence = get_next_command_list_done_fence();

                direct_command_queue->Signal(command_list_done_fence.Get(), CPU_FENCE_SIGNALED);

                const auto event = CreateEvent(nullptr, false, false, nullptr);
                command_list_done_fence->SetEventOnCompletion(CPU_FENCE_SIGNALED, event);

                WaitForSingleObject(event, INFINITE);

                retrieve_dred_report();

                command_list_done_fences.push_back(command_list_done_fence);

                CloseHandle(event);
            }
        }

        lists.clear();
    }

    void RenderDevice::return_staging_buffers_for_frame(const uint32_t frame_idx) {
        auto& staging_buffers_for_frame = staging_buffers_to_free[frame_idx];
        staging_buffers.insert(staging_buffers.end(), staging_buffers_for_frame.begin(), staging_buffers_for_frame.begin());
    }

    void RenderDevice::reset_command_allocators_for_frame(const uint32_t frame_idx) {
        direct_command_allocators[frame_idx]->Reset();
        copy_command_allocators[frame_idx]->Reset();
        compute_command_allocators[frame_idx]->Reset();
    }

    void RenderDevice::destroy_resources_for_frame(const uint32_t frame_idx) {
        auto& buffers = buffer_deletion_list[frame_idx];
        for(const auto& buffer : buffers) {
            destroy_resource_immediate(*buffer);
        }

        buffers.clear();

        auto& images = image_deletion_list[cur_gpu_frame_idx];
        for(const auto& image : images) {
            destroy_resource_immediate(*image);
        }
        images.clear();
    }

    void RenderDevice::transition_swapchain_image_to_render_target() {
        auto cmds = create_render_command_list();
        cmds->set_debug_name("Transition Swapchain to Render Target");
        auto* swapchain_cmds = dynamic_cast<D3D12CommandList*>(cmds.get());

        auto* cur_swapchain_image = swapchain_images[cur_swapchain_idx].Get();
        D3D12_RESOURCE_BARRIER swapchain_transition_barrier = CD3DX12_RESOURCE_BARRIER::Transition(cur_swapchain_image,
                                                                                                   D3D12_RESOURCE_STATE_COMMON,
                                                                                                   D3D12_RESOURCE_STATE_RENDER_TARGET);
        swapchain_cmds->get_command_list()->ResourceBarrier(1, &swapchain_transition_barrier);

        submit_command_list(std::move(cmds));
    }

    void RenderDevice::transition_swapchain_image_to_presentable() {
        auto cmds = create_render_command_list();
        cmds->set_debug_name("Transition Swapchain to Presentable");
        auto* swapchain_cmds = dynamic_cast<D3D12CommandList*>(cmds.get());

        auto* cur_swapchain_image = swapchain_images[cur_swapchain_idx].Get();
        D3D12_RESOURCE_BARRIER swapchain_transition_barrier = CD3DX12_RESOURCE_BARRIER::Transition(cur_swapchain_image,
                                                                                                   D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                                                   D3D12_RESOURCE_STATE_PRESENT);
        swapchain_cmds->get_command_list()->ResourceBarrier(1, &swapchain_transition_barrier);

        submit_command_list(std::move(cmds));
    }

    void RenderDevice::wait_for_frame(const uint64_t frame_index) {
        const auto desired_fence_value = frame_fence_values[frame_index];
        const auto initial_fence_value = frame_fences->GetCompletedValue();

        if(initial_fence_value < desired_fence_value) {
            // If the fence's most recent value is not the value we want, then the GPU has not finished executing the frame and we need to
            // explicitly wait

            frame_fences->SetEventOnCompletion(desired_fence_value, frame_event);
            const auto result = WaitForSingleObject(frame_event, INFINITE);
            if(result == WAIT_ABANDONED) {
                logger->error("Waiting for GPU frame {} was abandoned", frame_index);

            } else if(result == WAIT_TIMEOUT) {
                logger->error("Waiting for GPU frame {} timed out", frame_index);

            } else if(result == WAIT_FAILED) {
                logger->error("Waiting for GPU fence {} failed: {}", frame_index, get_last_windows_error());
            }

            ENSURE(result == WAIT_OBJECT_0, "Waiting for frame {} failed", frame_index);
        }
    }

    void RenderDevice::wait_gpu_idle(const uint64_t frame_index) {
        frame_fence_values[frame_index] += 3;
        direct_command_queue->Signal(frame_fences.Get(), frame_fence_values[frame_index]);
        wait_for_frame(frame_index);
    }

    StagingBuffer RenderDevice::create_staging_buffer(const uint32_t num_bytes) {
        MTR_SCOPE("RenderDevice", "create_buffer");

        const auto desc = CD3DX12_RESOURCE_DESC::Buffer(num_bytes);

        const D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_GENERIC_READ;

        D3D12MA::ALLOCATION_DESC alloc_desc{};
        alloc_desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        auto buffer = StagingBuffer{};
        const auto result = device_allocator->CreateResource(&alloc_desc,
                                                             &desc,
                                                             initial_state,
                                                             nullptr,
                                                             &buffer.allocation,
                                                             IID_PPV_ARGS(&buffer.resource));
        if(FAILED(result)) {
            logger->error("Could not create staging buffer: {}", to_string(result));
            return {};
        }

        buffer.size = num_bytes;
        D3D12_RANGE range{0, num_bytes};
        buffer.resource->Map(0, &range, &buffer.ptr);

        set_object_name(*buffer.resource.Get(), fmt::format("Staging Buffer {}", staging_buffer_idx));
        staging_buffer_idx++;

        return std::move(buffer);
    }

    D3D12Buffer RenderDevice::create_scratch_buffer(const uint32_t num_bytes) {
        constexpr auto alignment = std::max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                                            D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(num_bytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, alignment);

        const auto alloc_desc = D3D12MA::ALLOCATION_DESC{.HeapType = D3D12_HEAP_TYPE_DEFAULT};

        auto scratch_buffer = D3D12Buffer{};
        const auto result = device_allocator->CreateResource(&alloc_desc,
                                                             &desc,
                                                             D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                             nullptr,
                                                             &scratch_buffer.allocation,
                                                             IID_PPV_ARGS(&scratch_buffer.resource));
        if(FAILED(result)) {
            logger->error("Could not create scratch buffer: {}", to_string(result));
        }

        scratch_buffer.size = num_bytes;
        set_object_name(*scratch_buffer.resource.Get(), fmt::format("Scratch buffer {}", scratch_buffer_counter));
        scratch_buffer_counter++;

        return scratch_buffer;
    }

    ComPtr<ID3D12Fence> RenderDevice::get_next_command_list_done_fence() {
        if(!command_list_done_fences.empty()) {
            auto fence = command_list_done_fences[command_list_done_fences.size() - 1];
            command_list_done_fences.pop_back();

            return fence;
        }

        ComPtr<ID3D12Fence> fence;
        const auto result = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        if(FAILED(result)) {
            logger->error("Could not create fence: {}", to_string(result));
            const auto removed_reason = device->GetDeviceRemovedReason();
            logger->error("Device removed reason: {}", to_string(removed_reason));
        }

        return fence;
    }

    void RenderDevice::retrieve_dred_report() const {
        ComPtr<ID3D12DeviceRemovedExtendedData> dred;
        auto result = device->QueryInterface(IID_PPV_ARGS(&dred));
        if(FAILED(result)) {
            logger->error("Could not retrieve DRED report");
            return;
        }

        D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT breadcrumbs;
        result = dred->GetAutoBreadcrumbsOutput(&breadcrumbs);
        if(FAILED(result)) {
            return;
        }

        D3D12_DRED_PAGE_FAULT_OUTPUT page_faults;
        result = dred->GetPageFaultAllocationOutput(&page_faults);
        if(FAILED(result)) {
            return;
        }

        logger->error("Command history:\n{}", breadcrumb_output_to_string(breadcrumbs));
        logger->error(page_fault_output_to_string(page_faults));
    }

    std::unique_ptr<RenderDevice> make_render_device(const RenderBackend backend, GLFWwindow* window, const Settings& settings) {
        switch(backend) {
            case RenderBackend::D3D12: {
                const auto hwnd = glfwGetWin32Window(window);

                XMINT2 framebuffer_size{};
                glfwGetFramebufferSize(window, &framebuffer_size.x, &framebuffer_size.y);

                spdlog::info("Creating D3D12 backend");

                return std::make_unique<RenderDevice>(hwnd, framebuffer_size, settings);
            }
        }

        spdlog::error("Unrecognized render backend type");

        return {};
    }
} // namespace rhi
