#include "render_device.hpp"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <algorithm>

#include <D3D12MemAlloc.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <TracyD3D12.hpp>
#include <d3dcompiler.h>
#include <dxgi1_3.h>
#include <rx/core/abort.h>
#include <rx/core/log.h>

#include "adapters/tracy.hpp"
#include "core/constants.hpp"
#include "core/errors.hpp"
#include "rhi/compute_pipeline_state.hpp"
#include "rhi/d3dx12.hpp"
#include "rhi/helpers.hpp"
#include "rhi/render_pipeline_state.hpp"
#include "settings.hpp"
#include "windows/windows_helpers.hpp"

namespace renderer {
    RX_LOG("RenderDevice", logger);

    RenderDevice::RenderDevice(HWND window_handle,
                               const glm::uvec2& window_size,
                               const Settings& settings_in,
                               ftl::TaskScheduler& task_scheduler) // NOLINT(cppcoreguidelines-pro-type-member-init)
        : settings{settings_in},
          command_lists_by_frame{settings.num_in_flight_gpu_frames},
          buffer_deletion_list{settings.num_in_flight_gpu_frames},
          image_deletion_list{settings.num_in_flight_gpu_frames},
          staging_buffers_to_free{settings.num_in_flight_gpu_frames},
          scratch_buffers_to_free{settings.num_in_flight_gpu_frames},
          direct_command_allocators_fibtex{&task_scheduler},
          command_lists_by_frame_fibtex{&task_scheduler} {
#ifndef NDEBUG
        // Only enable the debug layer if we're not running in PIX
        const auto result = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&graphics_analysis));
        if(FAILED(result)) {
            enable_debugging();
        }
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

        create_standard_root_signature();

        create_material_resource_binders();

        create_pipeline_input_layouts();

        logger->info("Initialized D3D12 render device");
    }

    RenderDevice::~RenderDevice() {
        for(Uint32 i = 0; i < settings.num_in_flight_gpu_frames; i++) {
            wait_for_frame(i);
            direct_command_queue->Wait(frame_fences.Get(), frame_fence_values[i]);
        }

        wait_gpu_idle(0);

        staging_buffers.each_fwd([&](const Buffer& buffer) { buffer.allocation->Release(); });

        TracyD3D12Destroy(tracy_context);

        device_allocator->Release();
    }

    Rx::Ptr<Buffer> RenderDevice::create_buffer(const BufferCreateInfo& create_info) const {
        ZoneScoped;
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(create_info.size);

        if(create_info.usage == BufferUsage::StagingBuffer) {
            // Try to get a staging buffer from the pool
        }

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
                logger->warning("Unknown buffer usage %u", create_info.usage);
        }

        auto buffer = Rx::make_ptr<Buffer>(RX_SYSTEM_ALLOCATOR);
        const auto result = device_allocator->CreateResource(&alloc_desc,
                                                             &desc,
                                                             initial_state,
                                                             nullptr,
                                                             &buffer->allocation,
                                                             IID_PPV_ARGS(&buffer->resource));
        if(FAILED(result)) {
            logger->error("Could not create buffer %s: %s", create_info.name, to_string(result));
            return {};
        }

        if(should_map) {
            D3D12_RANGE mapped_range{0, create_info.size};
            buffer->resource->Map(0, &mapped_range, &buffer->mapped_ptr);
        }

        buffer->size = create_info.size;

        buffer->name = create_info.name;

        set_object_name(buffer->resource.Get(), create_info.name);

        return Rx::Utility::move(buffer);
    }

    Rx::Ptr<Image> RenderDevice::create_image(const ImageCreateInfo& create_info) const {
        auto format = to_dxgi_format(create_info.format); // TODO: Different to_dxgi_format functions for the different kinds of things
        if(format == DXGI_FORMAT_D32_FLOAT) {
            format = DXGI_FORMAT_R32_TYPELESS; // Create depth buffers with a TYPELESS format
        }
        auto desc = CD3DX12_RESOURCE_DESC::Tex2D(format,
                                                 static_cast<Uint32>(round(create_info.width)),
                                                 static_cast<Uint32>(round(create_info.height)));

        D3D12MA::ALLOCATION_DESC alloc_desc{.HeapType = D3D12_HEAP_TYPE_DEFAULT};

        if(create_info.enable_resource_sharing) {
            alloc_desc.ExtraHeapFlags |= D3D12_HEAP_FLAG_SHARED;
        }

        auto image = Rx::make_ptr<Image>(RX_SYSTEM_ALLOCATOR);
        image->format = create_info.format;

        D3D12_RESOURCE_STATES initial_state{};
        switch(create_info.usage) {
            case ImageUsage::RenderTarget:
                initial_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
                desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
                alloc_desc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED; // Render targets are always committed resources
                break;

            case ImageUsage::SampledImage:
                initial_state = D3D12_RESOURCE_STATE_COMMON;
                desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                break;

            case ImageUsage::DepthStencil:
                initial_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
                desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
                alloc_desc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED; // Depth/Stencil targets are always committed resources
                break;

            case ImageUsage::UnorderedAccess:
                initial_state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
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
        image->width = static_cast<Uint32>(desc.Width);
        image->height = desc.Height;

        set_object_name(image->resource.Get(), create_info.name);

        return image;
    }

    Rx::Ptr<Framebuffer> RenderDevice::create_framebuffer(const Rx::Vector<const Image*>& render_targets, const Image* depth_target) const {
        auto framebuffer = Rx::make_ptr<Framebuffer>(RX_SYSTEM_ALLOCATOR);

        framebuffer->render_targets = render_targets;
        framebuffer->depth_target = depth_target;

        Float32 width = 0;
        Float32 height = 0;

        framebuffer->rtv_handles.reserve(render_targets.size());
        Uint32 i{0};
        render_targets.each_fwd([&](const Image* image) {
            if(width != 0 && width != image->width) {
                logger->error(
                    "Render target %u has width %u, which is different from the width %u of the previous render target. All render targets must have the same width",
                    i,
                    image->width,
                    width);
            }

            width = static_cast<Float32>(image->width);

            if(height != 0 && height != image->height) {
                logger->error(
                    "Render target %u has height %u, which is different from the height %u of the previous render target. All render targets must have the same height",
                    i,
                    image->height,
                    height);
            }

            height = static_cast<Float32>(image->height);

            const auto handle = rtv_allocator->get_next_free_descriptor();

            device->CreateRenderTargetView(image->resource.Get(), nullptr, handle);

            framebuffer->rtv_handles.push_back(handle);

            i++;
        });

        if(depth_target != nullptr) {
            if(width != 0 && width != depth_target->width) {
                logger->error(
                    "Depth target %u has width %u, which is different from the width %u of the render targets. The depth target must have the same width as the render targets",
                    i,
                    depth_target->width,
                    width);
            }

            width = static_cast<Float32>(depth_target->width);

            if(height != 0 && height != depth_target->height) {
                logger->error(
                    "Depth target %u has height %u, which is different from the height %u of the render targets. The depth target must have the same height as the render targets",
                    i,
                    depth_target->height,
                    height);
            }

            height = static_cast<Float32>(depth_target->height);

            D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
            desc.Format = to_dxgi_format(depth_target->format);
            desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipSlice = 0;

            const auto handle = dsv_allocator->get_next_free_descriptor();

            device->CreateDepthStencilView(depth_target->resource.Get(), &desc, handle);

            framebuffer->dsv_handle = handle;
        }

        framebuffer->width = width;
        framebuffer->height = height;

        return framebuffer;
    }

    Framebuffer* RenderDevice::get_backbuffer_framebuffer() {
        const auto cur_swapchain_index = swapchain->GetCurrentBackBufferIndex();

        RX_ASSERT(cur_swapchain_index < swapchain_framebuffers.size(),
                  "Not enough swapchain framebuffers for current swapchain index %d",
                  cur_swapchain_index);

        return &swapchain_framebuffers[cur_swapchain_index];
    }

    void* RenderDevice::map_buffer(const Buffer& buffer) const {
        void* ptr;
        D3D12_RANGE range{0, buffer.size};
        const auto result = buffer.resource->Map(0, &range, &ptr);
        if(FAILED(result)) {
            logger->error("Could not map buffer");
            return nullptr;
        }

        return ptr;
    }

    void RenderDevice::schedule_buffer_destruction(Rx::Ptr<Buffer> buffer) {
        buffer_deletion_list[cur_gpu_frame_idx].emplace_back(RX_SYSTEM_ALLOCATOR, static_cast<Buffer*>(buffer.release()));
    }

    void RenderDevice::schedule_image_destruction(Rx::Ptr<Image> image) {
        image_deletion_list[cur_gpu_frame_idx].emplace_back(RX_SYSTEM_ALLOCATOR, static_cast<Image*>(image.release()));
    }

    void RenderDevice::destroy_framebuffer(const Rx::Ptr<Framebuffer> framebuffer) const {
        framebuffer->rtv_handles.each_fwd([&](const D3D12_CPU_DESCRIPTOR_HANDLE handle) { rtv_allocator->return_descriptor(handle); });

        if(framebuffer->dsv_handle) {
            dsv_allocator->return_descriptor(*framebuffer->dsv_handle);
        }
    }

    Rx::Ptr<BindGroupBuilder> RenderDevice::create_bind_group_builder(
        const Rx::Map<Rx::String, RootDescriptorDescription>& root_descriptors,
        const Rx::Map<Rx::String, DescriptorTableDescriptorDescription>& descriptor_table_descriptors,
        const Rx::Map<Uint32, D3D12_GPU_DESCRIPTOR_HANDLE>& descriptor_table_handles) {

        RX_ASSERT(descriptor_table_descriptors.is_empty() == descriptor_table_handles.is_empty(),
                  "If you specify descriptor table descriptors, you must also specify descriptor table handles");

        return Rx::make_ptr<BindGroupBuilder>(RX_SYSTEM_ALLOCATOR,
                                              *device.Get(),
                                              *cbv_srv_uav_heap.Get(),
                                              cbv_srv_uav_size,
                                              root_descriptors,
                                              descriptor_table_descriptors,
                                              descriptor_table_handles);
    }

    Rx::Ptr<ComputePipelineState> RenderDevice::create_compute_pipeline_state(const Rx::Vector<uint8_t>& compute_shader,
                                                                              const ComPtr<ID3D12RootSignature>& root_signature) const {
        auto compute_pipeline = Rx::make_ptr<ComputePipelineState>(RX_SYSTEM_ALLOCATOR);

        const auto desc = D3D12_COMPUTE_PIPELINE_STATE_DESC{
            .pRootSignature = root_signature.Get(),
            .CS =
                {
                    .pShaderBytecode = compute_shader.data(),
                    .BytecodeLength = compute_shader.size(),
                },
        };

        device->CreateComputePipelineState(&desc, IID_PPV_ARGS(compute_pipeline->pso.GetAddressOf()));

        compute_pipeline->root_signature = root_signature;

        return compute_pipeline;
    }

    Rx::Ptr<RenderPipelineState> RenderDevice::create_render_pipeline_state(const RenderPipelineStateCreateInfo& create_info) {
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
                logger->error("Unknown descriptor type, defaulting to UAV");
                return DescriptorType::UnorderedAccess;
        }
    }

    void RenderDevice::destroy_compute_pipeline_state(Rx::Ptr<ComputePipelineState> /* pipeline_state */) {
        // Nothing to explicitly do, the destructors will take care of us
    }

    void RenderDevice::destroy_render_pipeline_state(Rx::Ptr<RenderPipelineState> /* pipeline_state */) {
        // Nothing to do, destructors will take care of it
    }

    ComPtr<ID3D12GraphicsCommandList4> RenderDevice::create_command_list(const Size thread_idx) {
        auto* command_allocator = get_direct_command_allocator_for_thread(thread_idx);

        ComPtr<ID3D12GraphicsCommandList4> commands;
        ComPtr<ID3D12CommandList> cmds;
        const auto result = device->CreateCommandList(0,
                                                      D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                      command_allocator,
                                                      nullptr,
                                                      IID_PPV_ARGS(cmds.GetAddressOf()));
        if(FAILED(result)) {
            logger->error("Could not create command list");
            return {};
        }

        cmds->QueryInterface(commands.GetAddressOf());

        return commands;
    }

    void RenderDevice::submit_command_list(const ComPtr<ID3D12GraphicsCommandList4>& commands) {
        commands->Close();

        Rx::Concurrency::ScopeLock l{command_lists_by_frame_fibtex};
        command_lists_by_frame[cur_gpu_frame_idx].push_back(commands);
    }

    BindGroupBuilder& RenderDevice::get_material_bind_group_builder_for_frame(const Uint32 frame_idx) {
        RX_ASSERT(frame_idx < material_bind_group_builder.size(), "Not enough material resource binders for every swapchain image");

        return *material_bind_group_builder[frame_idx];
    }

    void RenderDevice::begin_frame(const uint64_t frame_count, const Size thread_idx) {
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

        transition_swapchain_image_to_render_target(thread_idx);

        in_init_phase = false;
    }

    void RenderDevice::end_frame(const Size thread_idx) {
        ZoneScoped;
        // Transition the swapchain image into the correct format and request presentation
        transition_swapchain_image_to_presentable(thread_idx);

        flush_batched_command_lists();

        direct_command_queue->Signal(frame_fences.Get(), frame_fence_values[cur_gpu_frame_idx]);

        {
            ZoneScopedN("present");
            const auto result = swapchain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
            if(result == DXGI_ERROR_DEVICE_HUNG || result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET) {
                logger->error("Device lost on present :(");

                if(settings.enable_gpu_crash_reporting) {
                    retrieve_dred_report();
                }
            }
        }

        cur_gpu_frame_idx = (cur_gpu_frame_idx + 1) % settings.num_in_flight_gpu_frames;
    }

    Uint32 RenderDevice::get_cur_gpu_frame_idx() const { return cur_gpu_frame_idx; }

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

    void RenderDevice::return_staging_buffer(Buffer&& buffer) { staging_buffers_to_free[cur_gpu_frame_idx].push_back(std::move(buffer)); }

    Buffer RenderDevice::get_scratch_buffer(const Uint32 num_bytes) {
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
            scratch_buffers.erase(best_fit_idx, best_fit_idx);

            return buffer;

        } else {
            return create_scratch_buffer(num_bytes);
        }
    }

    void RenderDevice::return_scratch_buffer(Buffer&& buffer) { scratch_buffers_to_free[cur_gpu_frame_idx].push_back(buffer); }

    UINT RenderDevice::get_shader_resource_descriptor_size() const { return cbv_srv_uav_size; }

    ID3D12Device* RenderDevice::get_d3d12_device() const { return device.Get(); }

    void RenderDevice::enable_debugging() {
        auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller));
        if(SUCCEEDED(result)) {
            debug_controller->EnableDebugLayer();

        } else {
            logger->error("Could not enable the D3D12 validation layer: {}", to_string(result).data());
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

    void RenderDevice::select_adapter() {
        ZoneScoped;

        // We want an adapter:
        // - Not integrated, if possible

        // TODO: Figure out how to get the number of adapters in advance
        Rx::Vector<ComPtr<IDXGIAdapter>> adapters;
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
                return true;
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

                    logger->warning("Ignoring adapter %s - Doesn't have the flexible resource binding that Sanity Engine needs",
                                    from_wide_string(desc.Description));

                    return true;
                }

                D3D12_FEATURE_DATA_SHADER_MODEL shader_model{D3D_SHADER_MODEL_6_5};
                res = try_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shader_model, sizeof(shader_model));
                if(FAILED(res)) {
                    logger->warning("Ignoring adapter %s - Could not check the supported shader model: %s",
                                    from_wide_string(desc.Description),
                                    to_string(res));

                    return true;

                } else if(shader_model.HighestShaderModel < D3D_SHADER_MODEL_6_5) {
                    // Only supports old-ass shaders

                    logger->warning("Ignoring adapter %s - Doesn't support the shader model Sanity Engine uses",
                                    from_wide_string(desc.Description));
                    return true;
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

                return false;

            } else {
                logger->warning("Ignoring adapter %s - doesn't support D3D12", from_wide_string(desc.Description));
            }

            return true;
        });

        if(!device) {
            Rx::abort("Could not find a suitable D3D12 adapter");
        }

        set_object_name(device.Get(), "D3D12 Device");
    }

    void RenderDevice::create_queues() {
        ZoneScoped;

        // One graphics queue and one optional DMA queue
        D3D12_COMMAND_QUEUE_DESC graphics_queue_desc{};
        graphics_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        graphics_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        graphics_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        auto result = device->CreateCommandQueue(&graphics_queue_desc, IID_PPV_ARGS(&direct_command_queue));
        if(FAILED(result)) {
            Rx::abort("Could not create graphics command queue");
        }

        set_object_name(direct_command_queue.Get(), "Direct Queue");

#ifdef TRACY_ENABLE
        tracy_context = TracyD3D12Context(device.Get(), direct_command_queue.Get());
#endif

        // TODO: Add an async compute queue, when the time comes

        if(!is_uma) {
            // No need to care about DMA on UMA cause we can just map everything
            D3D12_COMMAND_QUEUE_DESC dma_queue_desc{};
            dma_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
            dma_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            dma_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

            result = device->CreateCommandQueue(&dma_queue_desc, IID_PPV_ARGS(&async_copy_queue));
            if(FAILED(result)) {
                logger->warning("Could not create a DMA queue on a non-UMA adapter, data transfers will have to use the graphics queue");

            } else {
                set_object_name(async_copy_queue.Get(), "DMA queue");
            }
        }
    }

    void RenderDevice::create_swapchain(HWND window_handle, const glm::uvec2& window_size, const UINT num_images) {
        ZoneScoped;

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
            const auto msg = Rx::String::format("Could not create swapchain: %s", to_string(hr));
            Rx::abort(msg.data());
        }

        hr = swapchain1->QueryInterface(swapchain.GetAddressOf());
        if(FAILED(hr)) {
            Rx::abort("Could not get new swapchain interface, please update your drivers");
        }
    }

    void RenderDevice::create_gpu_frame_synchronization_objects() {
        frame_fence_values.resize(settings.num_in_flight_gpu_frames);

        device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frame_fences));
        set_object_name(frame_fences.Get(), "Frame Synchronization Fence");

        frame_event = CreateEvent(nullptr, false, false, nullptr);
    }

    void RenderDevice::create_command_allocators() {
        ZoneScoped;

        direct_command_allocators.resize(settings.num_in_flight_gpu_frames);
        compute_command_allocators.resize(settings.num_in_flight_gpu_frames);
        copy_command_allocators.resize(settings.num_in_flight_gpu_frames);

        for(Uint32 i = 0; i < settings.num_in_flight_gpu_frames; i++) {
            auto result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE,
                                                         IID_PPV_ARGS(compute_command_allocators[i].GetAddressOf()));
            if(FAILED(result)) {
                const auto msg = Rx::String::format("Could not create compute command allocator for frame %s", i);
                Rx::abort(msg.data());
            }

            set_object_name(compute_command_allocators[i].Get(), Rx::String::format("Compute Command Allocator %d", i));

            result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(copy_command_allocators[i].GetAddressOf()));
            if(FAILED(result)) {
                const auto msg = Rx::String::format("Could not create copy command allocator for frame %d", i);
                Rx::abort(msg.data());
            }

            set_object_name(copy_command_allocators[i].Get(), Rx::String::format("Copy Command Allocator %d", i));
        }
    }

    void RenderDevice::create_descriptor_heaps() {
        ZoneScoped;

        const auto [new_cbv_srv_uav_heap,
                    new_cbv_srv_uav_size] = create_descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                                   MAX_NUM_TEXTURES * 2 * settings.num_in_flight_gpu_frames);

        cbv_srv_uav_heap = new_cbv_srv_uav_heap;
        cbv_srv_uav_size = new_cbv_srv_uav_size;

        const auto [rtv_heap, rtv_size] = create_descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1024);
        rtv_allocator = Rx::make_ptr<DescriptorAllocator>(RX_SYSTEM_ALLOCATOR, rtv_heap, rtv_size);

        const auto [dsv_heap, dsv_size] = create_descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 32);
        dsv_allocator = Rx::make_ptr<DescriptorAllocator>(RX_SYSTEM_ALLOCATOR, dsv_heap, dsv_size);
    }

    void RenderDevice::initialize_swapchain_descriptors() {
        DXGI_SWAP_CHAIN_DESC1 desc;
        swapchain->GetDesc1(&desc);
        swapchain_images.resize(desc.BufferCount);
        swapchain_framebuffers.reserve(desc.BufferCount);

        for(Uint32 i = 0; i < desc.BufferCount; i++) {
            swapchain->GetBuffer(i, IID_PPV_ARGS(&swapchain_images[i]));

            const auto rtv_handle = rtv_allocator->get_next_free_descriptor();

            device->CreateRenderTargetView(swapchain_images[i].Get(), nullptr, rtv_handle);

            Framebuffer framebuffer;
            framebuffer.rtv_handles.push_back(rtv_handle);
            framebuffer.width = static_cast<Float32>(desc.Width);
            framebuffer.height = static_cast<Float32>(desc.Height);

            swapchain_framebuffers.push_back(std::move(framebuffer));

            set_object_name(swapchain_images[i].Get(), Rx::String::format("Swapchain image %d", i));
        }
    }

    std::pair<ID3D12DescriptorHeap*, UINT> RenderDevice::create_descriptor_heap(const D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type,
                                                                                const Uint32 num_descriptors) const {
        ID3D12DescriptorHeap* heap;

        D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
        heap_desc.Type = descriptor_type;
        heap_desc.NumDescriptors = num_descriptors;
        heap_desc.Flags = (descriptor_type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE :
                                                                                       D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

        const auto result = device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap));
        if(FAILED(result)) {
            logger->error("Could not create descriptor heap: %s", to_string(result));

            return {{}, 0};
        }

        const auto descriptor_size = device->GetDescriptorHandleIncrementSize(descriptor_type);

        return {heap, descriptor_size};
    }

    void RenderDevice::initialize_dma() {
        ZoneScoped;

        D3D12MA::ALLOCATOR_DESC allocator_desc{};
        allocator_desc.pDevice = device.Get();
        allocator_desc.pAdapter = adapter.Get();

        const auto result = D3D12MA::CreateAllocator(&allocator_desc, &device_allocator);
        if(FAILED(result)) {
            Rx::abort("Could not initialize DMA");
        }
    }

    void RenderDevice::create_standard_root_signature() {
        ZoneScoped;

        Rx::Vector<CD3DX12_ROOT_PARAMETER> root_parameters{9};

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

        // Index buffer
        root_parameters[5].InitAsShaderResourceView(4);

        // Vertex buffer
        root_parameters[6].InitAsShaderResourceView(5);

        // Per-frame data
        root_parameters[7].InitAsShaderResourceView(6);

        // Textures array
        Rx::Vector<D3D12_DESCRIPTOR_RANGE> descriptor_table_ranges;
        descriptor_table_ranges.push_back(D3D12_DESCRIPTOR_RANGE{
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            .NumDescriptors = MAX_NUM_TEXTURES,
            .BaseShaderRegister = 16,
            .RegisterSpace = 0,
            .OffsetInDescriptorsFromTableStart = 0,
        });

        root_parameters[8].InitAsDescriptorTable(static_cast<UINT>(descriptor_table_ranges.size()), descriptor_table_ranges.data());

        Rx::Vector<D3D12_STATIC_SAMPLER_DESC> static_samplers{3};

        // Point sampler
        auto& point_sampler = static_samplers[0];
        point_sampler = this->point_sampler_desc;

        auto& linear_sampler = static_samplers[1];
        linear_sampler = linear_sampler_desc;
        linear_sampler.ShaderRegister = 1;

        auto& trilinear_sampler = static_samplers[2];
        trilinear_sampler.Filter = D3D12_FILTER_ANISOTROPIC;
        trilinear_sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        trilinear_sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        trilinear_sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        trilinear_sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        trilinear_sampler.MaxAnisotropy = 8;
        trilinear_sampler.ShaderRegister = 2;

        D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
        root_signature_desc.NumParameters = static_cast<UINT>(root_parameters.size());
        root_signature_desc.pParameters = root_parameters.data();
        root_signature_desc.NumStaticSamplers = static_cast<UINT>(static_samplers.size());
        root_signature_desc.pStaticSamplers = static_samplers.data();
        root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        standard_root_signature = compile_root_signature(root_signature_desc);
        if(!standard_root_signature) {
            Rx::abort("Could not create standard root signature");
        }

        set_object_name(standard_root_signature.Get(), "Standard Root Signature");
    }

    ComPtr<ID3D12RootSignature> RenderDevice::compile_root_signature(const D3D12_ROOT_SIGNATURE_DESC& root_signature_desc) const {
        ZoneScoped;

        const auto versioned_desc = D3D12_VERSIONED_ROOT_SIGNATURE_DESC{.Version = D3D_ROOT_SIGNATURE_VERSION_1_0,
                                                                        .Desc_1_0 = root_signature_desc};

        ComPtr<ID3DBlob> root_signature_blob;
        ComPtr<ID3DBlob> error_blob;
        auto result = D3D12SerializeVersionedRootSignature(&versioned_desc, &root_signature_blob, &error_blob);
        if(FAILED(result)) {
            const Rx::String msg{static_cast<char*>(error_blob->GetBufferPointer()), error_blob->GetBufferSize()};
            logger->error("Could not create root signature: %s", msg);
            return {};
        }

        ComPtr<ID3D12RootSignature> sig;
        result = device->CreateRootSignature(0,
                                             root_signature_blob->GetBufferPointer(),
                                             root_signature_blob->GetBufferSize(),
                                             IID_PPV_ARGS(&sig));
        if(FAILED(result)) {
            logger->error("Could not create root signature: %s", to_string(result));
            return {};
        }

        return sig;
    }

    std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> RenderDevice::allocate_descriptor_table(
        const Uint32 num_descriptors) {
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle{cbv_srv_uav_heap->GetCPUDescriptorHandleForHeapStart(),
                                                 static_cast<INT>(next_free_cbv_srv_uav_descriptor),
                                                 cbv_srv_uav_size};
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle{cbv_srv_uav_heap->GetGPUDescriptorHandleForHeapStart(),
                                                 static_cast<INT>(next_free_cbv_srv_uav_descriptor),
                                                 cbv_srv_uav_size};

        next_free_cbv_srv_uav_descriptor += num_descriptors;

        return {cpu_handle, gpu_handle};
    }

    void RenderDevice::create_material_resource_binders() {
        Rx::Map<Rx::String, RootDescriptorDescription> root_descriptors;
        root_descriptors.insert("cameras", RootDescriptorDescription{1_u32, DescriptorType::ShaderResource});
        root_descriptors.insert("material_buffer", RootDescriptorDescription{2_u32, DescriptorType::ShaderResource});
        root_descriptors.insert("lights", RootDescriptorDescription{3_u32, DescriptorType::ShaderResource});
        root_descriptors.insert("raytracing_scene", RootDescriptorDescription{4_u32, DescriptorType::ShaderResource});
        root_descriptors.insert("indices", RootDescriptorDescription{5_u32, DescriptorType::ShaderResource});
        root_descriptors.insert("vertices", RootDescriptorDescription{6_u32, DescriptorType::ShaderResource});
        root_descriptors.insert("per_frame_data", RootDescriptorDescription{7_u32, DescriptorType::ShaderResource});

        material_bind_group_builder.reserve(settings.num_in_flight_gpu_frames);

        CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle{cbv_srv_uav_heap->GetCPUDescriptorHandleForHeapStart(),
                                                 static_cast<INT>(next_free_cbv_srv_uav_descriptor),
                                                 cbv_srv_uav_size};
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle{cbv_srv_uav_heap->GetGPUDescriptorHandleForHeapStart(),
                                                 static_cast<INT>(next_free_cbv_srv_uav_descriptor),
                                                 cbv_srv_uav_size};

        for(Uint32 i = 0; i < settings.num_in_flight_gpu_frames; i++) {
            Rx::Map<Rx::String, DescriptorTableDescriptorDescription> descriptor_tables;
            // Textures array _always_ is at the start of the descriptor heap
            descriptor_tables.insert("textures", DescriptorTableDescriptorDescription{DescriptorType::ShaderResource, cpu_handle});

            Rx::Map<Uint32, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_gpu_handles;
            descriptor_table_gpu_handles.insert(static_cast<Uint32>(root_descriptors.size() + 1), gpu_handle);

            material_bind_group_builder.push_back(
                create_bind_group_builder(root_descriptors, descriptor_tables, descriptor_table_gpu_handles));

            cpu_handle.Offset(MAX_NUM_TEXTURES, cbv_srv_uav_size);
            gpu_handle.Offset(MAX_NUM_TEXTURES, cbv_srv_uav_size);

            next_free_cbv_srv_uav_descriptor += MAX_NUM_TEXTURES;
        }
    }

    void RenderDevice::create_pipeline_input_layouts() {
        standard_graphics_pipeline_input_layout.reserve(5);

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
            D3D12_INPUT_ELEMENT_DESC{.SemanticName = "MaterialIndex",
                                     .SemanticIndex = 0,
                                     .Format = DXGI_FORMAT_R32_UINT,
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

        dear_imgui_graphics_pipeline_input_layout.reserve(2);

        dear_imgui_graphics_pipeline_input_layout.push_back(
            D3D12_INPUT_ELEMENT_DESC{.SemanticName = "Position",
                                     .SemanticIndex = 0,
                                     .Format = DXGI_FORMAT_R32G32_FLOAT,
                                     .InputSlot = 0,
                                     .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
                                     .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                     .InstanceDataStepRate = 0});

        dear_imgui_graphics_pipeline_input_layout.push_back(
            D3D12_INPUT_ELEMENT_DESC{.SemanticName = "Texcoord",
                                     .SemanticIndex = 0,
                                     .Format = DXGI_FORMAT_R32G32_FLOAT,
                                     .InputSlot = 0,
                                     .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
                                     .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                     .InstanceDataStepRate = 0});

        dear_imgui_graphics_pipeline_input_layout.push_back(
            D3D12_INPUT_ELEMENT_DESC{.SemanticName = "Color",
                                     .SemanticIndex = 0,
                                     .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                     .InputSlot = 0,
                                     .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
                                     .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                     .InstanceDataStepRate = 0});
    }

    Rx::Vector<D3D12_SHADER_INPUT_BIND_DESC> RenderDevice::get_bindings_from_shader(const Rx::Vector<uint8_t>& shader) const {
        ComPtr<ID3D12ShaderReflection> reflection;
        auto result = D3DReflect(shader.data(), shader.size() * sizeof(uint8_t), IID_PPV_ARGS(&reflection));
        if(FAILED(result)) {
            logger->error("Could not retrieve shader reflection information: %s", to_string(result));
        }

        D3D12_SHADER_DESC desc;
        result = reflection->GetDesc(&desc);
        if(FAILED(result)) {
            logger->error("Could not get shader description");
        }

        Rx::Vector<D3D12_SHADER_INPUT_BIND_DESC> input_descs(desc.BoundResources);

        for(Uint32 i = 0; i < desc.BoundResources; i++) {
            result = reflection->GetResourceBindingDesc(i, &input_descs[i]);
            if(FAILED(result)) {
                logger->error("Could not get binding information for resource idx %u", i);
            }
        }

        return input_descs;
    }

    Rx::Ptr<RenderPipelineState> RenderDevice::create_pipeline_state(const RenderPipelineStateCreateInfo& create_info,
                                                                     ID3D12RootSignature& root_signature) {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};

        desc.pRootSignature = &root_signature;

        desc.VS.BytecodeLength = create_info.vertex_shader.size();
        desc.VS.pShaderBytecode = create_info.vertex_shader.data();

        if(create_info.pixel_shader) {
            desc.PS.BytecodeLength = create_info.pixel_shader->size();
            desc.PS.pShaderBytecode = create_info.pixel_shader->data();
        }

        switch(create_info.input_assembler_layout) {
            case InputAssemblerLayout::StandardVertex:
                desc.InputLayout.NumElements = static_cast<UINT>(standard_graphics_pipeline_input_layout.size());
                desc.InputLayout.pInputElementDescs = standard_graphics_pipeline_input_layout.data();
                break;

            case InputAssemblerLayout::DearImGui:
                desc.InputLayout.NumElements = static_cast<UINT>(dear_imgui_graphics_pipeline_input_layout.size());
                desc.InputLayout.pInputElementDescs = dear_imgui_graphics_pipeline_input_layout.data();
                break;
        }
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
            for(Uint32 i = 0; i < blend_state.render_target_blends.size(); i++) {
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

        RX_ASSERT(create_info.render_target_formats.size() + (create_info.depth_stencil_format ? 1 : 0) > 0,
                  "Must have at least one render target or depth target");
        RX_ASSERT(create_info.render_target_formats.size() < 8,
                  "May not have more than 8 render targets - you have &d",
                  create_info.render_target_formats.size());

        desc.NumRenderTargets = static_cast<UINT>(create_info.render_target_formats.size());
        for(Uint32 i = 0; i < create_info.render_target_formats.size(); i++) {
            desc.RTVFormats[i] = to_dxgi_format(create_info.render_target_formats[i]);
        }
        if(create_info.depth_stencil_format) {
            desc.DSVFormat = to_dxgi_format(*create_info.depth_stencil_format);
        } else {
            desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
        }

        auto pipeline = Rx::make_ptr<RenderPipelineState>(RX_SYSTEM_ALLOCATOR);
        pipeline->root_signature = &root_signature;

        const auto result = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(pipeline->pso.GetAddressOf()));
        if(FAILED(result)) {
            logger->error("Could not create render pipeline %s: %s", create_info.name, to_string(result));
            return {};
        }

        set_object_name(pipeline->pso.Get(), create_info.name);

        return pipeline;
    }

    ID3D12CommandAllocator* RenderDevice::get_direct_command_allocator_for_thread(const Size id) {
        Rx::Concurrency::ScopeLock l{direct_command_allocators_fibtex};

        auto& command_allocators_for_frame = direct_command_allocators[cur_gpu_frame_idx];
        if(const auto* itr = command_allocators_for_frame.find(id)) {
            return itr->Get();

        } else {
            ComPtr<ID3D12CommandAllocator> allocator;
            const auto result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
            if(FAILED(result)) {
                Rx::abort("Could not create compute command allocator");

            } else {
                command_allocators_for_frame.insert(id, allocator);

                return allocator.Get();
            }
        }
    }

    void RenderDevice::flush_batched_command_lists() {
        Rx::Concurrency::ScopeLock l{command_lists_by_frame_fibtex};
        // Submit all the command lists we batched up
        auto& lists = command_lists_by_frame[cur_gpu_frame_idx];
        lists.each_fwd([&](ComPtr<ID3D12GraphicsCommandList4>& commands) {
            auto* d3d12_command_list = static_cast<ID3D12CommandList*>(commands.Get());

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
        });

        lists.clear();
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

    void RenderDevice::transition_swapchain_image_to_render_target(const Size thread_idx) {
        ZoneScoped;
        auto swapchain_cmds = create_command_list(thread_idx);
        set_object_name(swapchain_cmds.Get(), "Transition Swapchain to Render Target");

        {
            TracyD3D12Zone(tracy_context, swapchain_cmds.Get(), "Transition Swapchain to Render Target state");
            auto* cur_swapchain_image = swapchain_images[cur_swapchain_idx].Get();
            D3D12_RESOURCE_BARRIER swapchain_transition_barrier = CD3DX12_RESOURCE_BARRIER::Transition(cur_swapchain_image,
                                                                                                       D3D12_RESOURCE_STATE_COMMON,
                                                                                                       D3D12_RESOURCE_STATE_RENDER_TARGET);
            swapchain_cmds->ResourceBarrier(1, &swapchain_transition_barrier);
        }

        submit_command_list(swapchain_cmds);
    }

    void RenderDevice::transition_swapchain_image_to_presentable(const Size thread_idx) {
        auto swapchain_cmds = create_command_list(thread_idx);
        set_object_name(swapchain_cmds.Get(), "Transition Swapchain to Presentable");

        auto* cur_swapchain_image = swapchain_images[cur_swapchain_idx].Get();
        D3D12_RESOURCE_BARRIER swapchain_transition_barrier = CD3DX12_RESOURCE_BARRIER::Transition(cur_swapchain_image,
                                                                                                   D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                                                   D3D12_RESOURCE_STATE_PRESENT);
        swapchain_cmds->ResourceBarrier(1, &swapchain_transition_barrier);

        submit_command_list(swapchain_cmds);
    }

    void RenderDevice::wait_for_frame(const uint64_t frame_index) {
        ZoneScoped;
        const auto desired_fence_value = frame_fence_values[frame_index];
        const auto initial_fence_value = frame_fences->GetCompletedValue();

        if(initial_fence_value < desired_fence_value) {
            // If the fence's most recent value is not the value we want, then the GPU has not finished executing the frame and we need to
            // explicitly wait

            frame_fences->SetEventOnCompletion(desired_fence_value, frame_event);
            const auto result = WaitForSingleObject(frame_event, INFINITE);
            if(result == WAIT_ABANDONED) {
                logger->error("Waiting for GPU frame %u was abandoned", frame_index);

            } else if(result == WAIT_TIMEOUT) {
                logger->error("Waiting for GPU frame %u timed out", frame_index);

            } else if(result == WAIT_FAILED) {
                logger->error("Waiting for GPU fence %u failed: %s", frame_index, get_last_windows_error());
            }

            RX_ASSERT(result == WAIT_OBJECT_0, "Waiting for frame %d failed", frame_index);
        }
    }

    void RenderDevice::wait_gpu_idle(const uint64_t frame_index) {
        frame_fence_values[frame_index] += 3;
        direct_command_queue->Signal(frame_fences.Get(), frame_fence_values[frame_index]);
        wait_for_frame(frame_index);
    }

    Buffer RenderDevice::create_staging_buffer(const Uint32 num_bytes) {
        const auto desc = CD3DX12_RESOURCE_DESC::Buffer(num_bytes);

        const D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_GENERIC_READ;

        D3D12MA::ALLOCATION_DESC alloc_desc{};
        alloc_desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        auto buffer = Buffer{};
        auto result = device_allocator
                          ->CreateResource(&alloc_desc, &desc, initial_state, nullptr, &buffer.allocation, IID_PPV_ARGS(&buffer.resource));
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

        set_object_name(buffer.resource.Get(), Rx::String::format("Staging Buffer %d", staging_buffer_idx));
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

    ComPtr<ID3D12Fence> RenderDevice::get_next_command_list_done_fence() {
        if(!command_list_done_fences.is_empty()) {
            auto fence = command_list_done_fences[command_list_done_fences.size() - 1];
            command_list_done_fences.pop_back();

            return fence;
        }

        ComPtr<ID3D12Fence> fence;
        const auto result = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        if(FAILED(result)) {
            logger->error("Could not create fence: %s", to_string(result));
            const auto removed_reason = device->GetDeviceRemovedReason();
            logger->error("Device removed reason: %s", to_string(removed_reason));
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

        logger->error("Command history:\n%s", breadcrumb_output_to_string(breadcrumbs));
        logger->error(page_fault_output_to_string(page_faults).data());
    }

    Rx::Ptr<RenderDevice> make_render_device(const RenderBackend backend, GLFWwindow* window, const Settings& settings) {
        switch(backend) {
            case RenderBackend::D3D12: {
                auto hwnd = glfwGetWin32Window(window);

                glm::ivec2 framebuffer_size{};
                glfwGetFramebufferSize(window, &framebuffer_size.x, &framebuffer_size.y);

                logger->info("Creating D3D12 backend");

                auto* task_scheduler = Rx::Globals::find("SanityEngine")->find("TaskScheduler")->cast<ftl::TaskScheduler>();
                return Rx::make_ptr<RenderDevice>(RX_SYSTEM_ALLOCATOR, hwnd, framebuffer_size, settings, *task_scheduler);
            }
        }

        logger->error("Unrecognized render backend type");

        return {};
    }
} // namespace renderer
