/**
 * @file fluid_sim_pass.cpp
 *
 * @brief CPU-side implementation of a fluid simulation
 *
 * @note Mostly adapted from https://github.com/Scrawk/GPU-GEMS-3D-Fluid-Simulation/blob/master/Assets/FluidSim3D/Scripts/FireFluidSim.cs
 */

#include "fluid_sim_pass.hpp"

#include "Tracy.hpp"
#include "TracyD3D12.hpp"
#include "core/components.hpp"
#include "entt/entity/registry.hpp"
#include "loading/shader_loading.hpp"
#include "renderer/render_components.hpp"
#include "renderer/renderer.hpp"
#include "renderer/rhi/d3d12_private_data.hpp"

namespace sanity::engine::renderer {
    RX_CONSOLE_IVAR(
        num_pressure_iterations,
        "fluidSim.numPressureIterations",
        "Number of iterations for the pressure solver. Higher numbers give higher quality smoke and water at the expensive of runtime performance",
        1,
        32,
        10);

    RX_LOG("FluidSimPass", logger);

    static constexpr auto PARAMS_BUFFER_SIZE = MAX_NUM_FLUID_VOLUMES * sizeof(GpuFluidVolumeState);

    FluidSimPass::FluidSimPass(Renderer& renderer_in, const glm::uvec2& render_resolution)
        : renderer{&renderer_in},
          advection_params_array{"Fluid Sim Advection Params", PARAMS_BUFFER_SIZE, renderer_in},
          buoyancy_params_array{"Fluid Sim Buoyancy Params", PARAMS_BUFFER_SIZE, renderer_in},
          emitters_params_array{"Fluid Sim Emitter Params", PARAMS_BUFFER_SIZE, renderer_in},
          extinguishment_params_array{"Fluid Sim Extinguishment Params", PARAMS_BUFFER_SIZE, renderer_in},
          vorticity_confinement_params_array{"Fluid Sim Vorticity/Confinement Params", PARAMS_BUFFER_SIZE, renderer_in},
          divergence_params_array{"Fluid Sim Divergence Params", PARAMS_BUFFER_SIZE, renderer_in},
          projection_param_arrays{"Fluid Sim Projection Params", PARAMS_BUFFER_SIZE, renderer_in},
          rendering_params_array{"Fluid Sim Rendering Params", PARAMS_BUFFER_SIZE, renderer_in},
          fluid_sim_dispatch_command_buffers{"Fluid Sim Dispatch Commands", MAX_NUM_FLUID_VOLUMES * sizeof(FluidSimDispatch), renderer_in},
          drawcalls{"Fire Render Commands", MAX_NUM_FLUID_VOLUMES * sizeof(FluidSimDraw), renderer_in} {
        ZoneScoped;

        create_pipelines();

        create_indirect_command_signatures();

        pressure_param_arrays.reserve(*num_pressure_iterations);
        for(auto i = 0; i < *num_pressure_iterations; i++) {
            pressure_param_arrays.emplace_back(Rx::String::format("Fluid Sim Pressure Params iteration %d", i),
                                               static_cast<Uint32>(PARAMS_BUFFER_SIZE),
                                               renderer_in);
        }

        create_render_target(render_resolution);

        create_fluid_volume_geometry();

        set_resource_states();
    }

    void FluidSimPass::prepare_work(entt::registry& registry, const Uint32 frame_idx) {
        ZoneScoped;

        fluid_sim_draws.clear();
        fluid_sim_dispatches.clear();
        fluid_volume_states.clear();

        const auto& fluid_sims_view = registry.view<TransformComponent, FluidVolumeComponent>();
        if(fluid_sims_view.size() > MAX_NUM_FLUID_VOLUMES) {
            logger->error("Too many fluid volumes! Only %d are supported, you currently have %z",
                          MAX_NUM_FLUID_VOLUMES,
                          fluid_sims_view.size());
            return;

            // TODO: Don't error out here, simply cull the volumes such that there's no more than the max
        }

        fluid_sim_draws.reserve(fluid_sims_view.size());
        fluid_sim_dispatches.reserve(fluid_sims_view.size());
        fluid_volume_states.reserve(fluid_sims_view.size());

        fluid_sims_view.each(
            [&](const entt::entity& entity, const TransformComponent& transform, const FluidVolumeComponent& fluid_volume_component) {
                auto& fluid_volume = renderer->get_fluid_volume(fluid_volume_component.volume);

                const auto model_matrix_index = renderer->add_model_matrix_to_frame(transform.get_model_matrix(registry), frame_idx);
                const ObjectDrawData instance_data{.data_idx = fluid_volume_component.volume.index,
                                                   .entity_id = static_cast<Uint32>(entity),
                                                   .model_matrix_idx = model_matrix_index};

                add_fluid_volume_dispatch(fluid_volume, instance_data);
                add_fluid_volume_draw(fluid_volume, instance_data);
                add_fluid_volume_state(fluid_volume);
            });

        renderer->copy_data_to_buffer(fluid_sim_dispatch_command_buffers.get_active_resource(), fluid_sim_dispatches);
        drawcalls.get_all_resources().each_fwd([&](const BufferHandle& handle) { renderer->copy_data_to_buffer(handle, fluid_sim_draws); });
    }

    void FluidSimPass::record_work(ID3D12GraphicsCommandList4* commands, entt::registry& /* registry */, const Uint32 frame_idx) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_render_context, commands, "FluidSimPass::record_work");
        PIXScopedEvent(commands, PIX_COLOR(224, 96, 54), "FluidSimPass::record_work");

        auto& backend = renderer->get_render_backend();

        const auto& root_sig = backend.get_standard_root_signature();
        commands->SetComputeRootSignature(root_sig);

        auto* heap = renderer->get_render_backend().get_cbv_srv_uav_heap();
        commands->SetDescriptorHeaps(1, &heap);

        const auto array_descriptor = renderer->get_resource_array_gpu_descriptor(frame_idx);
        commands->SetComputeRootDescriptorTable(RenderBackend::RESOURCES_ARRAY_ROOT_PARAMETER_INDEX, array_descriptor);

        set_buffer_indices(commands, frame_idx);

        if(!fluid_volume_states.is_empty()) {
            record_fire_simulation_updates(commands, frame_idx);
        }

        // Record updates for other kinds of fluid volumes when I support them

        const auto anything_to_render = !fluid_volume_states.is_empty();
        if(anything_to_render) {
            TracyD3D12Zone(RenderBackend::tracy_render_context, commands, "render");
            PIXScopedEvent(commands, PIX_COLOR(224, 96, 54), "render");

            const auto& frame_constants_buffer = renderer->get_frame_constants_buffer(frame_idx);
            commands->SetGraphicsRoot32BitConstant(RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                                   frame_constants_buffer.index,
                                                   RenderBackend::FRAME_CONSTANTS_BUFFER_INDEX_ROOT_CONSTANT_OFFSET);

            const auto& model_matrix_buffer = renderer->get_model_matrix_for_frame(frame_idx);
            commands->SetGraphicsRoot32BitConstant(RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                                   model_matrix_buffer.index,
                                                   RenderBackend::MODEL_MATRIX_BUFFER_INDEX_ROOT_CONSTANT_OFFSET);

            commands->SetGraphicsRootDescriptorTable(RenderBackend::RESOURCES_ARRAY_ROOT_PARAMETER_INDEX, array_descriptor);

            commands->BeginRenderPass(1, &fluid_target_access, &depth_access, D3D12_RENDER_PASS_FLAG_NONE);

            if(!fluid_volume_states.is_empty()) {
                PIXScopedEvent(commands, PIX_COLOR(156, 57, 26), "fire");

                renderer->copy_data_to_buffer(rendering_params_array.get_active_resource(), fluid_sim_draws);

                commands->SetPipelineState(fire_fluid_pipeline->pso);

                const auto& render_texture = renderer->get_texture(fluid_color_texture);

                D3D12_VIEWPORT viewport{};
                viewport.MinDepth = 0;
                viewport.MaxDepth = 1;
                viewport.Width = static_cast<float>(render_texture.width);
                viewport.Height = static_cast<float>(render_texture.height);
                commands->RSSetViewports(1, &viewport);

                D3D12_RECT scissor_rect{};
                scissor_rect.right = static_cast<LONG>(render_texture.width);
                scissor_rect.bottom = static_cast<LONG>(render_texture.height);
                commands->RSSetScissorRects(1, &scissor_rect);

                commands->SetGraphicsRoot32BitConstant(RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                                       rendering_params_array.get_active_resource().index,
                                                       RenderBackend::DATA_BUFFER_INDEX_ROOT_PARAMETER_OFFSET);

                const auto& vertex_buffer_actual = renderer->get_buffer(cube_vertex_buffer);
                const D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view{.BufferLocation = vertex_buffer_actual->resource->GetGPUVirtualAddress(),
                                                                  .SizeInBytes = 6 * 4 * sizeof(float3),
                                                                  .StrideInBytes = sizeof(float3)};
                commands->IASetVertexBuffers(0, 1, &vertex_buffer_view);

                const auto& index_buffer_actual = renderer->get_buffer(cube_index_buffer);
                const D3D12_INDEX_BUFFER_VIEW index_buffer_view{.BufferLocation = index_buffer_actual->resource->GetGPUVirtualAddress(),
                                                                .SizeInBytes = static_cast<UINT>(index_buffer_actual->size),
                                                                .Format = DXGI_FORMAT_R32_UINT};
                commands->IASetIndexBuffer(&index_buffer_view);

                const auto argument_buffer_handle = drawcalls.get_active_resource();
                const auto& argument_buffer = renderer->get_buffer(argument_buffer_handle);
                commands->ExecuteIndirect(fluid_volume_draw_signature,
                                          static_cast<UINT>(fluid_volume_states.size()),
                                          argument_buffer->resource,
                                          0,
                                          nullptr,
                                          0);
            }

            commands->EndRenderPass();
        }

        // Always advance the arrays to the next frame so we can keep everything consistent
        advance_fire_sim_params_arrays();

        fluid_sim_dispatch_command_buffers.advance_frame();
    }

    TextureHandle FluidSimPass::get_color_target_handle() const { return fluid_color_texture; }

    void FluidSimPass::finalize_resources(ID3D12GraphicsCommandList* commands) {
        Rx::Vector<D3D12_RESOURCE_BARRIER> pre_copy_barriers;
        Rx::Vector<TextureCopyParams> copies;
        Rx::Vector<D3D12_RESOURCE_BARRIER> post_copy_barriers;

        if(*num_pressure_iterations % 2 == 1) {
            fluid_volume_states.each_fwd([&](GpuFluidVolumeState& state) {
                copy_read_texture_to_write_texture(state.pressure_textures[0],
                                                   state.pressure_textures[1],
                                                   pre_copy_barriers,
                                                   copies,
                                                   post_copy_barriers);
            });
        }

        if(!pre_copy_barriers.is_empty()) {
            commands->ResourceBarrier(static_cast<UINT>(pre_copy_barriers.size()), pre_copy_barriers.data());
        }

        copies.each_fwd(
            [&](const TextureCopyParams& params) { commands->CopyTextureRegion(&params.dest, 0, 0, 0, &params.source, nullptr); });

        if(!post_copy_barriers.is_empty()) {
            commands->ResourceBarrier(static_cast<UINT>(post_copy_barriers.size()), post_copy_barriers.data());
        }
    }

    void FluidSimPass::record_fire_simulation_updates(ID3D12GraphicsCommandList* commands, const Uint32 frame_idx) {
        TracyD3D12Zone(RenderBackend::tracy_render_context, commands, "FluidSimPass::record_fire_simulation_updates");
        PIXScopedEvent(commands, PIX_COLOR(224, 96, 54), "record_fire_simulation_updates");

        // Explanatory comments are from the original implementation at
        // https://github.com/Scrawk/GPU-GEMS-3D-Fluid-Simulation/blob/master/Assets/FluidSim3D/Scripts/FireFluidSim.cs, edited only to fix
        // typos

        // First off advect any buffers that contain physical quantities like density or temperature by the velocity field. Advection is
        // what moves values around.
        apply_advection(commands);

        // Apply the effect the sinking colder smoke has on the velocity field
        apply_buoyancy(commands);

        // Adds a certain amount of reaction (fire) and temperate
        apply_emitters(commands);

        // The smoke is formed when the reaction is extinguished. When the reaction amount falls below the extinguishment factor smoke is
        // added
        apply_extinguishment(commands);

        // The fluid sim math tends to remove the swirling movement of fluids.
        // This step will try and add it back in
        compute_vorticity_confinement(commands);

        // Compute the divergence of the velocity field. In fluid simulation the fluid is modeled as being incompressible meaning that the
        // volume of the fluid does not change over time. The divergence is the amount the field has deviated from being divergence free
        // [you mean compression free?]
        compute_divergence(commands);

        // This computes the pressure needed to return the fluid to a divergence free condition
        compute_pressure(commands);

        // Subtract the pressure field from the velocity field enforcing the divergence free conditions
        compute_projection(commands);

        // Final barriers to keep everything shipshape
        finalize_resources(commands);
    }

    void FluidSimPass::advance_fire_sim_params_arrays() {
        advection_params_array.advance_frame();
        buoyancy_params_array.advance_frame();
        emitters_params_array.advance_frame();
        extinguishment_params_array.advance_frame();
    }

    void FluidSimPass::create_pipelines() {
        create_simulation_pipelines();

        create_render_pipelines();
    }

    void FluidSimPass::create_simulation_pipelines() {
        auto& backend = renderer->get_render_backend();

        const auto advection_shader = load_shader("fluid/apply_advection.compute");
        advection_pipeline = backend.create_compute_pipeline_state(advection_shader);
        set_object_name(advection_pipeline, "Fluid Sim Advection");

        const auto buoyancy_shader = load_shader("fluid/apply_buoyancy.compute");
        buoyancy_pipeline = backend.create_compute_pipeline_state(buoyancy_shader);
        set_object_name(buoyancy_pipeline, "Fluid Sim Buoyancy");

        const auto emitters_shader = load_shader("fluid/apply_emitters.compute");
        emitters_pipeline = backend.create_compute_pipeline_state(emitters_shader);
        set_object_name(emitters_pipeline, "Fluid Sim Emitters");

        const auto extinguishment_shader = load_shader("fluid/apply_extinguishment.compute");
        extinguishment_pipeline = backend.create_compute_pipeline_state(extinguishment_shader);
        set_object_name(extinguishment_pipeline, "Fluid Sim Extinguishment");

        const auto vorticity_shader = load_shader("fluid/compute_vorticity.compute");
        vorticity_pipeline = backend.create_compute_pipeline_state(vorticity_shader);
        set_object_name(vorticity_pipeline, "Fluid Sim Vorticity");

        const auto confinement_shader = load_shader("fluid/compute_confinement.compute");
        confinement_pipeline = backend.create_compute_pipeline_state(confinement_shader);
        set_object_name(confinement_pipeline, "Fluid Sim Confinement");

        const auto divergence_shader = load_shader("fluid/compute_divergence.compute");
        divergence_pipeline = backend.create_compute_pipeline_state(divergence_shader);
        set_object_name(divergence_pipeline, "Fluid Sim Advection");

        const auto pressure_shader = load_shader("fluid/jacobi_pressure_solver.compute");
        jacobi_pressure_solver_pipeline = backend.create_compute_pipeline_state(pressure_shader);
        set_object_name(jacobi_pressure_solver_pipeline, "Fluid Sim Advection");

        const auto projection_shader = load_shader("fluid/compute_projection.compute");
        projection_pipeline = backend.create_compute_pipeline_state(projection_shader);
        set_object_name(projection_pipeline, "Fluid Sim Advection");
    }

    void FluidSimPass::create_render_pipelines() {
        auto& backend = renderer->get_render_backend();

        const auto vertex_shader = load_shader("standard.vertex");
        const auto pixel_shader = load_shader("fluid/fire.pixel");

        if(vertex_shader.is_empty() || pixel_shader.is_empty()) {
            logger->error("Could not load fire rendering pipelines");
            return;
        }

        fire_fluid_pipeline = backend.create_render_pipeline_state(
            RenderPipelineStateCreateInfo{.name = "Fire Render Pipeline",
                                          .vertex_shader = vertex_shader,
                                          .pixel_shader = pixel_shader,
                                          .input_assembler_layout = InputAssemblerLayout::StandardVertex,
                                          .blend_state = BlendState{.render_target_blends = {RenderTargetBlendState{.enabled = true},
                                                                                             RenderTargetBlendState{.enabled = false},
                                                                                             RenderTargetBlendState{.enabled = false},
                                                                                             RenderTargetBlendState{.enabled = false},
                                                                                             RenderTargetBlendState{.enabled = false},
                                                                                             RenderTargetBlendState{.enabled = false},
                                                                                             RenderTargetBlendState{.enabled = false},
                                                                                             RenderTargetBlendState{.enabled = false}}},
                                          .rasterizer_state = {},
                                          .depth_stencil_state = {.enable_depth_test = false, .enable_depth_write = false},
                                          .render_target_formats = Rx::Array{TextureFormat::Rgba16F},
                                          .depth_stencil_format = TextureFormat::Depth32});
    }

    void FluidSimPass::create_indirect_command_signatures() {
        auto& backend = renderer->get_render_backend();

        const auto dispatch_args = Rx::
            Array{D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT,
                                               .Constant = {.RootParameterIndex = RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                                            .DestOffsetIn32BitValues = RenderBackend::DATA_INDEX_ROOT_CONSTANT_OFFSET,
                                                            .Num32BitValuesToSet = 1}},
                  D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT,
                                               .Constant =
                                                   {.RootParameterIndex = RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                                    .DestOffsetIn32BitValues = RenderBackend::MODEL_MATRIX_INDEX_ROOT_CONSTANT_OFFSET,
                                                    .Num32BitValuesToSet = 1}},
                  D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT,
                                               .Constant = {.RootParameterIndex = RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                                            .DestOffsetIn32BitValues = RenderBackend::ENTITY_ID_ROOT_CONSTANT_OFFSET,
                                                            .Num32BitValuesToSet = 1}},
                  D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH}};

        const D3D12_COMMAND_SIGNATURE_DESC dispatch_command_desc{.ByteStride = static_cast<UINT>(sizeof(FluidSimDispatch)),
                                                                 .NumArgumentDescs = static_cast<UINT>(dispatch_args.size()),
                                                                 .pArgumentDescs = dispatch_args.data()};
        const auto& root_sig = backend.get_standard_root_signature();
        backend.device->CreateCommandSignature(&dispatch_command_desc, root_sig, IID_PPV_ARGS(&fluid_sim_dispatch_signature));

        const auto draw_args = Rx::
            Array{D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT,
                                               .Constant = {.RootParameterIndex = RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                                            .DestOffsetIn32BitValues = RenderBackend::DATA_INDEX_ROOT_CONSTANT_OFFSET,
                                                            .Num32BitValuesToSet = 1}},
                  D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT,
                                               .Constant =
                                                   {.RootParameterIndex = RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                                    .DestOffsetIn32BitValues = RenderBackend::MODEL_MATRIX_INDEX_ROOT_CONSTANT_OFFSET,
                                                    .Num32BitValuesToSet = 1}},
                  D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT,
                                               .Constant = {.RootParameterIndex = RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                                            .DestOffsetIn32BitValues = RenderBackend::ENTITY_ID_ROOT_CONSTANT_OFFSET,
                                                            .Num32BitValuesToSet = 1}},
                  D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED}};
        const D3D12_COMMAND_SIGNATURE_DESC draw_command_desc{.ByteStride = static_cast<UINT>(sizeof(FluidSimDraw)),
                                                             .NumArgumentDescs = static_cast<UINT>(draw_args.size()),
                                                             .pArgumentDescs = draw_args.data()};
        backend.device->CreateCommandSignature(&draw_command_desc, root_sig, IID_PPV_ARGS(&fluid_volume_draw_signature));
    }

    void FluidSimPass::create_render_target(const glm::uvec2& render_resolution) {
        fluid_color_texture = renderer->create_texture(TextureCreateInfo{
            .name = "Fluid Volume Render Target",
            .usage = TextureUsage::RenderTarget,
            .format = TextureFormat::Rgba16F,
            .width = render_resolution.x,
            .height = render_resolution.y,
            .depth = 1,
        });
        const auto& render_target = renderer->get_texture(fluid_color_texture);
        auto& backend = renderer->get_render_backend();
        fluid_color_rtv = backend.create_rtv_handle(render_target);
        fluid_target_access = {.cpuDescriptor = fluid_color_rtv.cpu_handle,
                               .BeginningAccess = {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
                                                   .Clear = {.ClearValue = {.Format = DXGI_FORMAT_R32_FLOAT, .Color = {0, 0, 0, 0}}}},
                               .EndingAccess = {.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE}};

        const auto& depth_target_handle = renderer->get_depth_buffer();
        const auto& depth_target = renderer->get_texture(depth_target_handle);
        const auto& depth_descriptor = backend.create_dsv_handle(depth_target);
        depth_access = D3D12_RENDER_PASS_DEPTH_STENCIL_DESC{
            .cpuDescriptor = depth_descriptor.cpu_handle,
            .DepthBeginningAccess = {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE},
            .StencilBeginningAccess = {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS},
            .DepthEndingAccess = {.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE},
            .StencilEndingAccess = {.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS},
        };

        set_resource_usage(fluid_color_texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
        set_resource_usage(depth_target_handle, D3D12_RESOURCE_STATE_DEPTH_READ);
    }

    void FluidSimPass::create_fluid_volume_geometry() {
        // Our fluid volume render pipelines only care about vertex location, so we can ignore the other attributes for now

        // +z face, then -z face, in proper winding order
        const auto cube_vertices = Rx::Array{
            StandardVertex{.location = {0.5f, 0.f, 0.5f}},
            StandardVertex{.location = {0.5f, 1.f, 0.5f}},
            StandardVertex{.location = {-0.5f, 1.f, 0.5f}},
            StandardVertex{.location = {-0.5f, 0.f, 0.5f}},

            StandardVertex{.location = {-0.5f, 0.f, -0.5f}},
            StandardVertex{.location = {-0.5f, 1.f, -0.5f}},
            StandardVertex{.location = {0.5f, 1.f, -0.5f}},
            StandardVertex{.location = {0.5f, 0.f, -0.5f}},
        };

        const auto cube_indices = Rx::Array{// +z
                                            0,
                                            1,
                                            2,
                                            1,
                                            2,
                                            3,

                                            // -z
                                            4,
                                            5,
                                            6,
                                            5,
                                            6,
                                            7,

                                            // +x
                                            7,
                                            6,
                                            1,
                                            6,
                                            1,
                                            0,

                                            // -x
                                            4,
                                            5,
                                            2,
                                            5,
                                            2,
                                            3,

                                            // +y
                                            1,
                                            6,
                                            5,
                                            6,
                                            5,
                                            2,

                                            // -y
                                            4,
                                            3,
                                            0,
                                            3,
                                            0,
                                            7};

        cube_vertex_buffer = renderer->create_buffer(BufferCreateInfo{.name = "Fluid Volume Vertices",
                                                                      .usage = BufferUsage::VertexBuffer,
                                                                      .size = static_cast<Uint32>(cube_vertices.size() *
                                                                                                  sizeof(StandardVertex))},
                                                     cube_vertices.data());
        cube_index_buffer = renderer->create_buffer(BufferCreateInfo{.name = "Fluid Volume Indices",
                                                                     .usage = BufferUsage::IndexBuffer,
                                                                     .size = static_cast<Uint32>(cube_indices.size() * sizeof(Uint32))},
                                                    cube_indices.data());
    }

    void FluidSimPass::set_resource_states() {
        set_resource_usage(fluid_color_texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
        set_resource_usage(advection_params_array.get_all_resources(),
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        set_resource_usage(buoyancy_params_array.get_all_resources(),
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        set_resource_usage(emitters_params_array.get_all_resources(),
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        set_resource_usage(extinguishment_params_array.get_all_resources(),
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        set_resource_usage(vorticity_confinement_params_array.get_all_resources(),
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        set_resource_usage(divergence_params_array.get_all_resources(),
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        set_resource_usage(projection_param_arrays.get_all_resources(),
                           D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        set_resource_usage(fluid_sim_dispatch_command_buffers.get_all_resources(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

        set_resource_usage(cube_vertex_buffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        set_resource_usage(cube_index_buffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);
        set_resource_usage(rendering_params_array.get_all_resources(),
                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        set_resource_usage(drawcalls.get_all_resources(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
    }

    void FluidSimPass::add_fluid_volume_dispatch(const FluidVolume& fluid_volume, const ObjectDrawData& instance_data) {
        const auto& voxel_size = fluid_volume.get_voxel_size();
        fluid_sim_dispatches.push_back(FluidSimDispatch{.data_idx = instance_data.data_idx,
                                                        .model_matrix_idx = instance_data.model_matrix_idx,
                                                        .entity_id = instance_data.entity_id,
                                                        .thread_group_count_x = voxel_size.x / FLUID_SIM_NUM_THREADS,
                                                        .thread_group_count_y = voxel_size.y / FLUID_SIM_NUM_THREADS,
                                                        .thread_group_count_z = voxel_size.z / FLUID_SIM_NUM_THREADS});
    }

    void FluidSimPass::add_fluid_volume_draw(const FluidVolume& fluid_volume, const ObjectDrawData& instance_data) {
        fluid_sim_draws.push_back(FluidSimDraw{.data_idx = instance_data.data_idx,
                                               .model_matrix_idx = instance_data.model_matrix_idx,
                                               .entity_id = instance_data.entity_id,
                                               .index_count = 24,
                                               .instance_count = 1,
                                               .first_index = 0,
                                               .first_vertex = 0,
                                               .first_instance = 0});
    }

    void FluidSimPass::add_fluid_volume_state(const FluidVolume& fluid_volume) {
        const auto& density_textures = fluid_volume.density_texture;
        const auto& temperature_textures = fluid_volume.temperature_texture;
        const auto& reaction_textures = fluid_volume.reaction_texture;
        const auto& velocity_textures = fluid_volume.velocity_texture;
        const auto& pressure_textures = fluid_volume.pressure_texture;
        const auto& temp_texture = fluid_volume.temp_texture;

        // We don't need to clear the texture states from the previous frame, since we're using the same resources each frame

        // TODO: Once culling is working, any volumes that shouldn't get updated for a given frame need their states removed

        const Rx::Vector<TextureHandle> read_textures = Rx::Array{density_textures[0],
                                                                  temperature_textures[0],
                                                                  reaction_textures[0],
                                                                  velocity_textures[0],
                                                                  pressure_textures[0]};
        set_resource_usages(read_textures, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        const Rx::Vector<TextureHandle> write_textures = Rx::Array{
            density_textures[1],
            temperature_textures[1],
            reaction_textures[1],
            velocity_textures[1],
            pressure_textures[1],
            temp_texture,
        };
        set_resource_usages(write_textures, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        const GpuFluidVolumeState initial_state{.density_textures = {density_textures[0], density_textures[1]},
                                                .temperature_textures = {temperature_textures[0], temperature_textures[1]},
                                                .reaction_textures = {reaction_textures[0], reaction_textures[1]},
                                                .velocity_textures = {velocity_textures[0], velocity_textures[1]},
                                                .pressure_textures = {pressure_textures[0], pressure_textures[1]},
                                                .temp_data_buffer = temp_texture,
                                                .size = {fluid_volume.size, 0.f},
                                                .dissipation = {fluid_volume.density_dissipation,
                                                                fluid_volume.temperature_dissipation,
                                                                1.0,
                                                                fluid_volume.velocity_dissipation},
                                                .decay = {0.f, 0.f, fluid_volume.reaction_decay, 0.f},
                                                .buoyancy = fluid_volume.buoyancy,
                                                .weight = fluid_volume.weight,
                                                .emitter_location = {fluid_volume.emitter_location, 0.f},
                                                .emitter_radius = fluid_volume.emitter_radius,
                                                .emitter_strength = fluid_volume.emitter_strength,
                                                .reaction_extinguishment = fluid_volume.reaction_extinguishment,
                                                .density_extinguishment_amount = fluid_volume.density_extinguishment_amount,
                                                .vorticity_strength = fluid_volume.vorticity_strength};

        fluid_volume_states.push_back(initial_state);
    }

    void FluidSimPass::set_buffer_indices(ID3D12GraphicsCommandList* commands, const Uint32 frame_idx) const {
        const auto& frame_constants_buffer = renderer->get_frame_constants_buffer(frame_idx);
        commands->SetComputeRoot32BitConstant(RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                              frame_constants_buffer.index,
                                              RenderBackend::FRAME_CONSTANTS_BUFFER_INDEX_ROOT_CONSTANT_OFFSET);

        const auto& model_matrix_buffer = renderer->get_model_matrix_for_frame(frame_idx);
        commands->SetComputeRoot32BitConstant(RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                              model_matrix_buffer.index,
                                              RenderBackend::MODEL_MATRIX_BUFFER_INDEX_ROOT_CONSTANT_OFFSET);
    }

    void FluidSimPass::execute_simulation_step(
        ID3D12GraphicsCommandList* commands,
        const BufferRing& data_buffer,
        const ComPtr<ID3D12PipelineState>& pipeline,
        const Rx::Function<void(GpuFluidVolumeState&, Rx::Vector<D3D12_RESOURCE_BARRIER>& barriers)> synchronize_volume) {
        const auto data_buffer_handle = data_buffer.get_active_resource();
        renderer->copy_data_to_buffer(data_buffer_handle, fluid_volume_states);

        commands->SetPipelineState(pipeline);

        commands->SetComputeRoot32BitConstant(RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                              data_buffer_handle.index,
                                              RenderBackend::DATA_BUFFER_INDEX_ROOT_PARAMETER_OFFSET);

        const auto& dispatch_buffer_handle = fluid_sim_dispatch_command_buffers.get_active_resource();
        const auto& dispatch_buffer = renderer->get_buffer(dispatch_buffer_handle);
        commands->ExecuteIndirect(fluid_sim_dispatch_signature,
                                  static_cast<UINT>(fluid_sim_dispatches.size()),
                                  dispatch_buffer->resource,
                                  0,
                                  nullptr,
                                  0);

        Rx::Vector<D3D12_RESOURCE_BARRIER> barriers;
        barriers.reserve(fluid_volume_states.size());

        fluid_volume_states.each_fwd([&](GpuFluidVolumeState& state) { synchronize_volume(state, barriers); });

        if(!barriers.is_empty()) {
            commands->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
        }
    }

    void FluidSimPass::apply_advection(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_render_context, commands, "Advection");
        PIXScopedEvent(commands, PIX_COLOR_DEFAULT, "Advection");

        execute_simulation_step(commands, advection_params_array, advection_pipeline, [&](GpuFluidVolumeState& volume, auto& barriers) {
            barrier_and_swap(volume.density_textures, barriers);
            barrier_and_swap(volume.temperature_textures, barriers);
            barrier_and_swap(volume.reaction_textures, barriers);
            barrier_and_swap(volume.velocity_textures, barriers);
        });
    }

    void FluidSimPass::apply_buoyancy(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_render_context, commands, "Bouyancy");
        PIXScopedEvent(commands, PIX_COLOR_DEFAULT, "Bouyancy");

        execute_simulation_step(commands, buoyancy_params_array, buoyancy_pipeline, [&](GpuFluidVolumeState& state, auto& barriers) {
            barrier_and_swap(state.velocity_textures, barriers);
        });
    }

    void FluidSimPass::apply_emitters(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_render_context, commands, "Impulse");
        PIXScopedEvent(commands, PIX_COLOR_DEFAULT, "Impulse");

        execute_simulation_step(commands, emitters_params_array, emitters_pipeline, [&](GpuFluidVolumeState& state, auto& barriers) {
            barrier_and_swap(state.reaction_textures, barriers);
            barrier_and_swap(state.temperature_textures, barriers);
        });
    }

    void FluidSimPass::apply_extinguishment(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_render_context, commands, "Extinguishment");
        PIXScopedEvent(commands, PIX_COLOR_DEFAULT, "Extinguishment");

        execute_simulation_step(commands,
                                extinguishment_params_array,
                                extinguishment_pipeline,
                                [&](GpuFluidVolumeState& state, auto& barriers) { barrier_and_swap(state.density_textures, barriers); });
    }

    void FluidSimPass::compute_vorticity_confinement(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_render_context, commands, "Vorticity Confinement");
        PIXScopedEvent(commands, PIX_COLOR_DEFAULT, "Vorticity Confinement");

        execute_simulation_step(commands,
                                vorticity_confinement_params_array,
                                vorticity_pipeline,
                                [&](GpuFluidVolumeState& state, auto& barriers) {
                                    const auto& temp_data_texture = renderer->get_texture(state.temp_data_buffer);
                                    barriers.push_back(
                                        CD3DX12_RESOURCE_BARRIER::Transition(temp_data_texture.resource,
                                                                             D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                             D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
                                });

        execute_simulation_step(commands,
                                vorticity_confinement_params_array,
                                confinement_pipeline,
                                [&](GpuFluidVolumeState& state, auto& barriers) {
                                    barrier_and_swap(state.velocity_textures, barriers);

                                    const auto& temp_data_texture = renderer->get_texture(state.temp_data_buffer);
                                    barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(temp_data_texture.resource,
                                                                                            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                                                                            D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
                                });
    }

    void FluidSimPass::compute_divergence(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_render_context, commands, "Divergence");
        PIXScopedEvent(commands, PIX_COLOR_DEFAULT, "Divergence");

        execute_simulation_step(commands, divergence_params_array, divergence_pipeline, [&](GpuFluidVolumeState& state, auto& barriers) {
            const auto& temp_data_texture = renderer->get_texture(state.temp_data_buffer);
            barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(temp_data_texture.resource,
                                                                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
        });
    }

    void FluidSimPass::compute_pressure(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_render_context, commands, "Pressure");
        PIXScopedEvent(commands, PIX_COLOR_DEFAULT, "Pressure");

        for(auto i = 0; i < *num_pressure_iterations; i++) {
            execute_simulation_step(commands,
                                    pressure_param_arrays[i],
                                    jacobi_pressure_solver_pipeline,
                                    [&](GpuFluidVolumeState& state, auto& barriers) {
                                        barrier_and_swap(state.pressure_textures, barriers);
                                    });
        }

        Rx::Vector<D3D12_RESOURCE_BARRIER> barriers;
        barriers.reserve(fluid_volume_states.size());
        fluid_volume_states.each_fwd([&](const GpuFluidVolumeState& state) {
            const auto& temp_data_texture = renderer->get_texture(state.temp_data_buffer);
            barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(temp_data_texture.resource,
                                                                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                                                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
        });

        commands->ResourceBarrier(static_cast<Uint32>(barriers.size()), barriers.data());
    }

    void FluidSimPass::compute_projection(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_render_context, commands, "Projection");
        PIXScopedEvent(commands, PIX_COLOR_DEFAULT, "Projection");

        execute_simulation_step(commands, projection_param_arrays, projection_pipeline, [&](GpuFluidVolumeState& state, auto& barriers) {
            barrier_and_swap(state.velocity_textures, barriers);
        });
    }

    void FluidSimPass::barrier_and_swap(TextureHandle handles[2], Rx::Vector<D3D12_RESOURCE_BARRIER>& barriers) const {
        const auto& old_read_texture = renderer->get_texture(handles[0]);
        const auto& old_write_texture = renderer->get_texture(handles[1]);

        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(old_read_texture.resource,
                                                                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                                                D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(old_write_texture.resource,
                                                                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

        const auto handle = handles[0];
        handles[0] = handles[1];
        handles[1] = handle;
    }

    void FluidSimPass::copy_read_texture_to_write_texture(const TextureHandle read,
                                                          const TextureHandle write,
                                                          Rx::Vector<D3D12_RESOURCE_BARRIER>& pre_copy_barriers,
                                                          Rx::Vector<TextureCopyParams>& copies,
                                                          Rx::Vector<D3D12_RESOURCE_BARRIER>& post_copy_barriers) const {
        const auto old_read_texture = renderer->get_texture(read);
        const auto old_write_texture = renderer->get_texture(write);
        pre_copy_barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(old_read_texture.resource,
                                                                         D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                                                         D3D12_RESOURCE_STATE_COPY_SOURCE));
        pre_copy_barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(old_write_texture.resource,
                                                                         D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                         D3D12_RESOURCE_STATE_COPY_DEST));

        copies.emplace_back(D3D12_TEXTURE_COPY_LOCATION{.pResource = old_read_texture.resource,
                                                        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                                                        .SubresourceIndex = 0},
                            D3D12_TEXTURE_COPY_LOCATION{.pResource = old_write_texture.resource,
                                                        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                                                        .SubresourceIndex = 0});

        post_copy_barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(old_read_texture.resource,
                                                                          D3D12_RESOURCE_STATE_COPY_SOURCE,
                                                                          D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
        post_copy_barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(old_write_texture.resource,
                                                                          D3D12_RESOURCE_STATE_COPY_DEST,
                                                                          D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

        logger->verbose("Transitioning %s to shader resource, %s to unordered access", old_write_texture.name, old_read_texture.name);
    }
} // namespace sanity::engine::renderer
