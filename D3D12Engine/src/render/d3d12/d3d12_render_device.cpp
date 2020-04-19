#include "d3d12_render_device.hpp"

#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <minitrace.h>
#include <spdlog/spdlog.h>

#include "../../core/abort.hpp"
#include "../../core/constants.hpp"
#include "d3d12_compute_command_list.hpp"
#include "d3d12_framebuffer.hpp"
#include "d3d12_render_command_list.hpp"
#include "d3d12_resource_command_list.hpp"
#include "d3dx12.hpp"
#include "helpers.hpp"
#include "resources.hpp"

using std::move;

namespace render {

    D3D12RenderDevice::D3D12RenderDevice(const HWND window_handle, const XMINT2& window_size) {
#ifndef NDEBUG
        enable_validation_layer();
#endif

        initialize_dxgi();

        select_adapter();

        create_queues();

        create_swapchain(window_handle, window_size, 1);

        create_command_allocators();

        create_descriptor_heaps();

        // initialize_swapchain_descriptors();

        initialize_dma();

        create_standard_root_signature();

        create_material_resource_binder();

        create_standard_graphics_pipeline_input_layout();

        command_completion_thread = std::make_unique<std::thread>(&D3D12RenderDevice::wait_for_command_lists, this);
    }

    D3D12RenderDevice::~D3D12RenderDevice() { device_allocator->Release(); }

    std::unique_ptr<Buffer> D3D12RenderDevice::create_buffer(const BufferCreateInfo& create_info) {
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

        auto buffer = std::make_unique<D3D12Buffer>();
        const auto result = device_allocator->CreateResource(&alloc_desc,
                                                             &desc,
                                                             initial_state,
                                                             nullptr,
                                                             &buffer->allocation,
                                                             IID_PPV_ARGS(&buffer->resource));
        if(FAILED(result)) {
            spdlog::error("Could not create buffer %s", create_info.name);
            return {};
        }

        buffer->size = create_info.size;

        set_object_name(*buffer->resource.Get(), create_info.name);

        return move(buffer);
    }

    std::unique_ptr<Image> D3D12RenderDevice::create_image(const ImageCreateInfo& create_info) {
        MTR_SCOPE("D3D12RenderDevice", "create_image");

        const auto format = to_dxgi_format(create_info.format);
        const auto desc = CD3DX12_RESOURCE_DESC::Tex2D(format, round(create_info.width), round(create_info.height));

        D3D12MA::ALLOCATION_DESC alloc_desc{};
        alloc_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        auto image = std::make_unique<D3D12Image>();
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

            spdlog::warn("Unrecognized usage for image {}, defaulting to the common resource state", create_info.name);
            return D3D12_RESOURCE_STATE_COMMON;
        }();

        const auto result = device_allocator->CreateResource(&alloc_desc,
                                                             &desc,
                                                             initial_state,
                                                             nullptr,
                                                             &image->allocation,
                                                             IID_PPV_ARGS(&image->resource));
        if(FAILED(result)) {
            spdlog::error("Could not create image %s", create_info.name);
            return {};
        }

        set_object_name(*image->resource.Get(), create_info.name);

        return image;
    }

    std::unique_ptr<Framebuffer> D3D12RenderDevice::create_framebuffer(const std::vector<const Image*>& render_targets,
                                                                       const Image* depth_target) {
        MTR_SCOPE("D3D12RenderDevice", "create_framebuffer");

        auto framebuffer = std::make_unique<D3D12Framebuffer>();

        framebuffer->rtv_handles.reserve(render_targets.size());
        for(const auto* image : render_targets) {
            const auto* d3d12_image = static_cast<const D3D12Image*>(image);

            D3D12_RENDER_TARGET_VIEW_DESC desc{};
            desc.Format = d3d12_image->format;
            desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            desc.Texture2D.PlaneSlice = 0;
            desc.Texture2D.MipSlice = 0;

            const auto handle = rtv_allocator->get_next_free_descriptor();

            device->CreateRenderTargetView(d3d12_image->resource.Get(), &desc, handle);

            framebuffer->rtv_handles.push_back(handle);
        }

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
            spdlog::error("Could not map buffer");
            return nullptr;
        }

        return ptr;
    }

    void D3D12RenderDevice::destroy_buffer(std::unique_ptr<Buffer> /* buffer */) {
        // We don't need to do anything special, D3D12 has destructors
    }

    void D3D12RenderDevice::destroy_image(std::unique_ptr<Image> /* image */) {
        // Again nothing to do, D3D12 still has destructors
    }

    void D3D12RenderDevice::destroy_framebuffer(const std::unique_ptr<Framebuffer> framebuffer) {
        auto* d3d12_framebuffer = static_cast<D3D12Framebuffer*>(framebuffer.get());

        for(const D3D12_CPU_DESCRIPTOR_HANDLE handle : d3d12_framebuffer->rtv_handles) {
            rtv_allocator->return_descriptor(handle);
        }

        if(d3d12_framebuffer->dsv_handle) {
            dsv_allocator->return_descriptor(*d3d12_framebuffer->dsv_handle);
        }
    }

    std::unique_ptr<ComputePipelineState> D3D12RenderDevice::create_compute_pipeline_state(const std::vector<uint8_t>& compute_shader) {
        auto compute_pipeline = std::make_unique<D3D12ComputePipelineState>();

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
        desc.CS.BytecodeLength = compute_shader.size();
        desc.CS.pShaderBytecode = compute_shader.data();

        device->CreateComputePipelineState(&desc, IID_PPV_ARGS(compute_pipeline->pso.GetAddressOf()));

        return compute_pipeline;
    }

    std::unique_ptr<RenderPipelineState> D3D12RenderDevice::create_render_pipeline_state(const RenderPipelineStateCreateInfo& create_info) {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};

        if(create_info.use_standard_material_layout) {
            desc.pRootSignature = standard_root_signature.Get();
        }

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
            output_rasterizer_state.DepthBias = rasterizer_state.depth_bias; // TODO: Figure out what the actual fuck D3D12 depth bias is
            output_rasterizer_state.DepthBiasClamp = rasterizer_state.max_depth_bias;
            output_rasterizer_state.SlopeScaledDepthBias = rasterizer_state.slope_scaled_depth_bias;
            output_rasterizer_state.MultisampleEnable = rasterizer_state.num_msaa_samples > 0 ? 1 : 0;
            output_rasterizer_state.AntialiasedLineEnable = rasterizer_state.enable_line_antialiasing;
            output_rasterizer_state.ConservativeRaster = rasterizer_state.enable_conservative_rasterization ?
                                                             D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON :
                                                             D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

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

        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

        auto pipeline = std::make_unique<D3D12RenderPipelineState>();
        if(create_info.use_standard_material_layout) {
            pipeline->root_signature = standard_root_signature;
        }

        device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline->pso));

        return pipeline;
    }

    void D3D12RenderDevice::destroy_compute_pipeline_state(std::unique_ptr<ComputePipelineState> /* pipeline_state */) {
        // Nothing to explicitly do, the destructors will take care of us
    }

    void D3D12RenderDevice::destroy_render_pipeline_state(std::unique_ptr<RenderPipelineState> /* pipeline_state */) {
        // Nothing to do, destructors will take care of it
    }

    std::unique_ptr<ResourceCommandList> D3D12RenderDevice::create_resource_command_list() {
        MTR_SCOPE("D3D12RenderDevice", "get_resoruce_command_list");

        ComPtr<ID3D12GraphicsCommandList> commands;
        ComPtr<ID3D12CommandList> cmds;
        const auto result = device->CreateCommandList(0,
                                                      D3D12_COMMAND_LIST_TYPE_COPY,
                                                      copy_command_allocator.Get(),
                                                      nullptr,
                                                      IID_PPV_ARGS(cmds.GetAddressOf()));
        if(FAILED(result)) {
            spdlog::error("Could not create resource command list");
            return {};
        }

        cmds->QueryInterface(commands.GetAddressOf());

        return std::make_unique<D3D12ResourceCommandList>(commands, *this);
    }

    std::unique_ptr<ComputeCommandList> D3D12RenderDevice::create_compute_command_list() {
        MTR_SCOPE("D3D12RenderDevice", "get_compute_command_list");

        ComPtr<ID3D12GraphicsCommandList> commands;
        ComPtr<ID3D12CommandList> cmds;
        const auto result = device->CreateCommandList(0,
                                                      D3D12_COMMAND_LIST_TYPE_COMPUTE,
                                                      compute_command_allocator.Get(),
                                                      nullptr,
                                                      IID_PPV_ARGS(cmds.GetAddressOf()));
        if(FAILED(result)) {
            spdlog::error("Could not create compute command list");
            return {};
        }

        cmds->QueryInterface(commands.GetAddressOf());

        return std::make_unique<D3D12ComputeCommandList>(commands, *this);
    }

    std::unique_ptr<RenderCommandList> D3D12RenderDevice::create_render_command_list() {
        MTR_SCOPE("D3D12RenderDevice", "create_graphics_command_list");

        ComPtr<ID3D12GraphicsCommandList> commands;
        ComPtr<ID3D12CommandList> cmds;
        const auto result = device->CreateCommandList(0,
                                                      D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                      direct_command_allocator.Get(),
                                                      nullptr,
                                                      IID_PPV_ARGS(cmds.GetAddressOf()));
        if(FAILED(result)) {
            spdlog::error("Could not create render command list");
            return {};
        }

        cmds->QueryInterface(commands.GetAddressOf());

        return std::make_unique<D3D12RenderCommandList>(commands, *this);
    }

    void D3D12RenderDevice::submit_command_list(std::unique_ptr<CommandList> commands) {
        auto* d3d12_commands = dynamic_cast<D3D12CommandList*>(commands.get());

        auto* d3d12_command_list = d3d12_commands->get_command_list();

        // First implementation - run everything on the same queue, because it's easy
        // Eventually I'll come up with a fancy way to use multiple queues

        direct_command_queue->ExecuteCommandLists(1, &d3d12_command_list);

        auto command_list_done_fence = get_next_command_list_done_fence();

        direct_command_queue->Signal(command_list_done_fence.Get(), CPU_FENCE_SIGNALED);

        {
            std::lock_guard m{in_flight_command_lists_mutex};
            in_flight_command_lists.emplace(command_list_done_fence, dynamic_cast<D3D12CommandList*>(commands.release()));
        }

        commands_lists_in_flight_cv.notify_one();
    }

    void D3D12RenderDevice::begin_frame() {
        {
            std::lock_guard l{done_command_lists_mutex};
            while(!done_command_lists.empty()) {
                auto* list = done_command_lists.front();
                done_command_lists.pop();

                list->execute_completion_functions();
                delete list;
            }
        }
    }

    bool D3D12RenderDevice::has_separate_device_memory() const { return !is_uma; }

    D3D12StagingBuffer D3D12RenderDevice::get_staging_buffer(const size_t num_bytes) {
        size_t best_fit_idx = staging_buffers.size();
        for(size_t i = 0; i < staging_buffers.size(); i++) {
            if(staging_buffers[i].size >= num_bytes) {
                if(best_fit_idx >= staging_buffers.size()) {
                    // This is the first suitable buffer we've found
                    best_fit_idx = i;

                } else if(staging_buffers[i].size < staging_buffers[best_fit_idx].size) {
                    // The current buffer is more suitable than the previous best buffer
                    best_fit_idx = i;
                }
            }
        }

        if(best_fit_idx < staging_buffers.size()) {
            // We found a valid staging buffer!
            auto buffer = std::move(staging_buffers[best_fit_idx]);
            staging_buffers.erase(staging_buffers.begin() + best_fit_idx);
            return buffer;

        } else {
            // No suitable buffer is available, let's make a new one
            return create_staging_buffer(num_bytes);
        }
    }

    void D3D12RenderDevice::return_staging_buffer(D3D12StagingBuffer&& buffer) { staging_buffers.push_back(std::move(buffer)); }

    ComPtr<ID3D12Fence> D3D12RenderDevice::get_next_command_list_done_fence() {
        if(!command_list_done_fences.empty()) {
            auto fence = *command_list_done_fences.end();
            command_list_done_fences.pop_back();

            return fence;
        }

        ComPtr<ID3D12Fence> fence;
        device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()));

        return fence;
    }

    UINT D3D12RenderDevice::get_shader_resource_descriptor_size() const { return cbv_srv_uav_size; }

    ID3D12Device* D3D12RenderDevice::get_d3d12_device() const { return device.Get(); }

    void D3D12RenderDevice::enable_validation_layer() {
        const auto res = D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller));
        if(SUCCEEDED(res)) {
            debug_controller->EnableDebugLayer();

        } else {
            spdlog::error("Could not enable the D3D12 validation layer");
        }
    }

    void D3D12RenderDevice::initialize_dxgi() {
        MTR_SCOPE("D3D12RenderDevice", "initialize_dxgi");

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

    void D3D12RenderDevice::select_adapter() {
        MTR_SCOPE("D3D12RenderDevice", "select_adapter");

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

                    continue;
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

#ifndef DEBUG
                res = device->QueryInterface(info_queue.GetAddressOf());
                if(SUCCEEDED(res)) {
                    info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
                    info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                }
#endif

                break;
            }

            continue;
        }

        if(!device) {
            critical_error("Could not find a suitable D3D12 adapter");
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
            critical_error("Could not create graphics command queue");
        }

        set_object_name(*direct_command_queue.Get(), "Direct Queue");

        // TODO: Add an async compute queue, when the time comes

        if(!is_uma) {
            // No need to care about DMA on UMA cause we can just map everything
            D3D12_COMMAND_QUEUE_DESC dma_queue_desc{};
            dma_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
            result = device->CreateCommandQueue(&dma_queue_desc, IID_PPV_ARGS(&async_copy_queue));
            if(FAILED(result)) {
                spdlog::warn("Could not create a DMA queue on a non-UMA adapter, data transfers will have to use the graphics queue");

            } else {
                set_object_name(*async_copy_queue.Get(), "DMA queue");
            }
        }
    }

    void D3D12RenderDevice::create_swapchain(HWND window_handle, const XMINT2& window_size, const UINT num_images) {
        MTR_SCOPE("D3D12RenderDevice", "create_swapchain");
        DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
        swapchain_desc.Width = static_cast<UINT>(window_size.x);
        swapchain_desc.Height = static_cast<UINT>(window_size.y);
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
            const auto msg = fmt::format("Could not create swapchain: {}", hr);
            critical_error(msg.data());
        }

        hr = swapchain1->QueryInterface(swapchain.GetAddressOf());
        if(FAILED(hr)) {
            critical_error("Could not get new swapchain interface, please update your drivers");
        }
    }

    void D3D12RenderDevice::create_command_allocators() {
        MTR_SCOPE("D3D12RenderDevice", "create_command_allocators");

        auto result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&direct_command_allocator));
        if(FAILED(result)) {
            critical_error("Could not create direct command allocator");
        }

        result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(compute_command_allocator.GetAddressOf()));
        if(FAILED(result)) {
            critical_error("Could not create compute command allocator");
        }

        result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&copy_command_allocator));
        if(FAILED(result)) {
            critical_error("Could not create copy command allocator");
        }
    }

    void D3D12RenderDevice::create_descriptor_heaps() {
        MTR_SCOPE("D3D12RenderDevice", "create_descriptor_heaps");
        const auto& [new_cbv_srv_uav_heap, new_cbv_srv_uav_size] = create_descriptor_allocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                                                                               65536);
        cbv_srv_uav_heap = new_cbv_srv_uav_heap;
        cbv_srv_uav_size = new_cbv_srv_uav_size;

        const auto& [rtv_heap, rtv_size] = create_descriptor_allocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1024);
        rtv_allocator = std::make_unique<D3D12DescriptorAllocator>(rtv_heap, rtv_size);

        const auto& [dsv_heap, dsv_size] = create_descriptor_allocator(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 32);
        dsv_allocator = std::make_unique<D3D12DescriptorAllocator>(dsv_heap, dsv_size);
    }

    std::pair<ComPtr<ID3D12DescriptorHeap>, UINT> D3D12RenderDevice::create_descriptor_allocator(
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

    void D3D12RenderDevice::initialize_dma() {
        MTR_SCOPE("D3D12RenderDevice", "iniitialize_dma");

        D3D12MA::ALLOCATOR_DESC allocator_desc{};
        allocator_desc.pDevice = device.Get();
        allocator_desc.pAdapter = adapter.Get();

        const auto result = D3D12MA::CreateAllocator(&allocator_desc, &device_allocator);
        if(FAILED(result)) {
            critical_error("Could not initialize DMA");
        }
    }

    void D3D12RenderDevice::create_standard_root_signature() {
        MTR_SCOPE("D3D12RenderDevice", "create_standard_root_signature");

        std::vector<CD3DX12_ROOT_PARAMETER> root_parameters{4};

        // Root constants for material index and camera index
        root_parameters[0].InitAsConstants(2, 0);

        // Camera data buffer
        root_parameters[1].InitAsShaderResourceView(0);

        // Material data buffer
        root_parameters[2].InitAsShaderResourceView(1);

        // Textures array
        std::vector<D3D12_DESCRIPTOR_RANGE> descriptor_table_ranges;
        D3D12_DESCRIPTOR_RANGE textures_array;
        textures_array.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        textures_array.NumDescriptors = MAX_NUM_TEXTURES;
        textures_array.BaseShaderRegister = 3;
        textures_array.RegisterSpace = 0;
        textures_array.OffsetInDescriptorsFromTableStart = 0;
        descriptor_table_ranges.push_back(move(textures_array));

        root_parameters[3].InitAsDescriptorTable(static_cast<UINT>(descriptor_table_ranges.size()), descriptor_table_ranges.data());

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

        D3D12_ROOT_SIGNATURE_DESC root_signature_desc{};
        root_signature_desc.NumParameters = static_cast<UINT>(root_parameters.size());
        root_signature_desc.pParameters = root_parameters.data();
        root_signature_desc.NumStaticSamplers = static_cast<UINT>(static_samplers.size());
        root_signature_desc.pStaticSamplers = static_samplers.data();

        standard_root_signature = compile_root_signature(root_signature_desc);
        if(!standard_root_signature) {
            critical_error("Could not create standard root signature");
        }

        set_object_name(*standard_root_signature.Get(), "Standard Root Signature");
    }

    ComPtr<ID3D12RootSignature> D3D12RenderDevice::compile_root_signature(const D3D12_ROOT_SIGNATURE_DESC& root_signature_desc) const {
        MTR_SCOPE("D3D12RenderDevice", "compile_root_signature");

        ComPtr<ID3DBlob> root_signature_blob;
        ComPtr<ID3DBlob> error_blob;
        auto result = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_signature_blob, &error_blob);
        if(FAILED(result)) {
            const std::string msg{reinterpret_cast<char*>(error_blob->GetBufferPointer()), error_blob->GetBufferSize()};
            spdlog::error("Could not create root signature: %s", msg);
            return {};
        }

        ComPtr<ID3D12RootSignature> sig;
        result = device->CreateRootSignature(0,
                                             root_signature_blob->GetBufferPointer(),
                                             root_signature_blob->GetBufferSize(),
                                             IID_PPV_ARGS(&sig));
        if(FAILED(result)) {
            spdlog::error("Could not create root signature");
            return {};
        }

        return sig;
    }

    void D3D12RenderDevice::create_material_resource_binder() {}

    void D3D12RenderDevice::create_standard_graphics_pipeline_input_layout() {
        standard_graphics_pipeline_input_layout.reserve(5);

        standard_graphics_pipeline_input_layout.push_back(
            D3D12_INPUT_ELEMENT_DESC{/* .SemanticName = */ "Position",
                                     /* .SemanticIndex = */ 0,
                                     /* .Format = */ DXGI_FORMAT_R32G32B32_FLOAT,
                                     /* .InputSlot = */ 0,
                                     /* .AlignedByteOffset = */ D3D12_APPEND_ALIGNED_ELEMENT,
                                     /* .InputSlotClass = */ D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                     /* .InstanceDataStepRate = */ 0});

        standard_graphics_pipeline_input_layout.push_back(
            D3D12_INPUT_ELEMENT_DESC{/* .SemanticName = */ "Normal",
                                     /* .SemanticIndex = */ 0,
                                     /* .Format = */ DXGI_FORMAT_R32G32B32_FLOAT,
                                     /* .InputSlot = */ 0,
                                     /* .AlignedByteOffset = */ D3D12_APPEND_ALIGNED_ELEMENT,
                                     /* .InputSlotClass = */ D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                     /* .InstanceDataStepRate = */ 0});

        standard_graphics_pipeline_input_layout.push_back(
            D3D12_INPUT_ELEMENT_DESC{/* .SemanticName = */ "Color",
                                     /* .SemanticIndex = */ 0,
                                     /* .Format = */ DXGI_FORMAT_R8G8B8A8_UNORM,
                                     /* .InputSlot = */ 0,
                                     /* .AlignedByteOffset = */ D3D12_APPEND_ALIGNED_ELEMENT,
                                     /* .InputSlotClass = */ D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                     /* .InstanceDataStepRate = */ 0});

        standard_graphics_pipeline_input_layout.push_back(
            D3D12_INPUT_ELEMENT_DESC{/* .SemanticName = */ "Texcoord",
                                     /* .SemanticIndex = */ 0,
                                     /* .Format = */ DXGI_FORMAT_R32G32_FLOAT,
                                     /* .InputSlot = */ 0,
                                     /* .AlignedByteOffset = */ D3D12_APPEND_ALIGNED_ELEMENT,
                                     /* .InputSlotClass = */ D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                     /* .InstanceDataStepRate = */ 0});

        standard_graphics_pipeline_input_layout.push_back(
            D3D12_INPUT_ELEMENT_DESC{/* .SemanticName = */ "DoubleSided",
                                     /* .SemanticIndex = */ 0,
                                     /* .Format = */ DXGI_FORMAT_R32_UINT,
                                     /* .InputSlot = */ 0,
                                     /* .AlignedByteOffset = */ D3D12_APPEND_ALIGNED_ELEMENT,
                                     /* .InputSlotClass = */ D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                     /* .InstanceDataStepRate = */ 0});
    }

    D3D12StagingBuffer D3D12RenderDevice::create_staging_buffer(const size_t num_bytes) const {
        MTR_SCOPE("D3D12RenderDevice", "create_buffer");

        const auto desc = CD3DX12_RESOURCE_DESC::Buffer(num_bytes);

        const D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_GENERIC_READ;

        D3D12MA::ALLOCATION_DESC alloc_desc{};
        alloc_desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        D3D12StagingBuffer buffer;
        const auto result = device_allocator->CreateResource(&alloc_desc,
                                                             &desc,
                                                             initial_state,
                                                             nullptr,
                                                             &buffer.allocation,
                                                             IID_PPV_ARGS(&buffer.resource));
        if(FAILED(result)) {
            spdlog::error("Could not create staging buffer");
            return {};
        }

        buffer.size = num_bytes;
        ;

        set_object_name(*buffer.resource.Get(), "Staging Buffer");

        return buffer;
    }

    void D3D12RenderDevice::wait_for_command_lists(D3D12RenderDevice* render_device) {
        const HANDLE event = CreateEvent(nullptr, false, false, nullptr);

        bool should_wait_for_cv = false;

        while(true) {
            if(should_wait_for_cv) {
                std::unique_lock l{render_device->in_flight_command_lists_mutex};
                render_device->commands_lists_in_flight_cv.wait(l, [&] { return !render_device->in_flight_command_lists.empty(); });

                should_wait_for_cv = false;
            }

            std::pair<ComPtr<ID3D12Fence>, D3D12CommandList*> cur_pair;
            {
                std::lock_guard l{render_device->in_flight_command_lists_mutex};
                if(render_device->in_flight_command_lists.empty()) {
                    should_wait_for_cv = true;
                    continue;
                }

                cur_pair = std::move(render_device->in_flight_command_lists.front());
                render_device->in_flight_command_lists.pop();
            }

            cur_pair.first->SetEventOnCompletion(CPU_FENCE_SIGNALED, event);
            WaitForSingleObject(event, 2000);

            {
                std::lock_guard l{render_device->done_command_lists_mutex};
                render_device->done_command_lists.emplace(std::move(cur_pair.second));
            }
        }
    }
} // namespace render
