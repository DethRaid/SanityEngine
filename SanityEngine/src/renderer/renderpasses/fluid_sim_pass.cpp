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

    FluidSimPass::FluidSimPass(Renderer& renderer_in)
        : renderer{&renderer_in},
          advection_params_array{"Fluid Sim Advection Params", PARAMS_BUFFER_SIZE, renderer_in},
          buoyancy_params_array{"Fluid Sim Buoyancy Params", PARAMS_BUFFER_SIZE, renderer_in},
          emitters_params_array{"Fluid Sim Emitter Params", PARAMS_BUFFER_SIZE, renderer_in},
          extinguishment_params_array{"Fluid Sim Extinguishment Params", PARAMS_BUFFER_SIZE, renderer_in},
          vorticity_confinement_params_array{"Fluid Sim Vorticity/Confinement Params", PARAMS_BUFFER_SIZE, renderer_in},
          divergence_params_array{"Fluid Sim Divergence Params", PARAMS_BUFFER_SIZE, renderer_in},
          projection_param_arrays{"Fluid Sim Projection Params", PARAMS_BUFFER_SIZE, renderer_in},
          fluid_sim_dispatch_command_buffers{"Fluid Sim Dispatch Commands", MAX_NUM_FLUID_VOLUMES * sizeof(FluidSimDispatch), renderer_in} {
        ZoneScoped;

        load_shaders();

        create_indirect_command_signature();

        pressure_param_arrays.reserve(*num_pressure_iterations);
        for(auto i = 0; i < *num_pressure_iterations; i++) {
            pressure_param_arrays.emplace_back(Rx::String::format("Fluid Sim Pressure Params iteration %d", i),
                                               static_cast<Uint32>(PARAMS_BUFFER_SIZE),
                                               renderer_in);
        }
    }

    void FluidSimPass::collect_work(entt::registry& registry, const Uint32 frame_idx) {
        ZoneScoped;

        fluid_sim_dispatches.clear();
        fluid_volume_states.clear();

        const auto& fluid_sims_view = registry.view<TransformComponent, FluidVolumeComponent>();
        fluid_sim_dispatches.reserve(fluid_sims_view.size());
        fluid_volume_states.reserve(fluid_sims_view.size());

        fluid_sims_view.each(
            [&](const entt::entity& entity, const TransformComponent& transform, const FluidVolumeComponent& fluid_volume_component) {
                const auto model_matrix_index = renderer->add_model_matrix_to_frame(transform.get_model_matrix(registry), frame_idx);

                auto& fluid_volume = renderer->get_fluid_volume(fluid_volume_component.volume);

                const auto& voxel_size = fluid_volume.get_voxel_size();
                const auto dispatch = FluidSimDispatch{.fluid_volume_idx = fluid_volume_component.volume.index,
                                                       .entity_id = static_cast<uint>(entity),
                                                       .model_matrix_idx = model_matrix_index,
                                                       .thread_group_count_x = voxel_size.x / FLUID_SIM_NUM_THREADS,
                                                       .thread_group_count_y = voxel_size.y / FLUID_SIM_NUM_THREADS,
                                                       .thread_group_count_z = voxel_size.z / FLUID_SIM_NUM_THREADS};
                fluid_sim_dispatches.push_back(dispatch);

                const auto& density_textures = fluid_volume.density_texture;
                const auto& temperature_textures = fluid_volume.temperature_texture;
                const auto& reaction_textures = fluid_volume.reaction_texture;
                const auto& velocity_textures = fluid_volume.velocity_texture;
                const auto& pressure_textures = fluid_volume.pressure_texture;
                const auto& temp_texture = fluid_volume.temp_texture;

                const auto initial_state = GpuFluidVolumeState{.density_textures = {density_textures[0], density_textures[1]},
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
            });

        renderer->copy_data_to_buffer(fluid_sim_dispatch_command_buffers.get_active_resource(), fluid_sim_dispatches);
    }

    void FluidSimPass::record_work(ID3D12GraphicsCommandList4* commands, entt::registry& registry, const Uint32 frame_idx) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "FluidSimPass::record_work");
        PIXScopedEvent(commands, PIX_COLOR(224, 96, 54), "FluidSimPass::record_work");

        auto& backend = renderer->get_render_backend();

        const auto& root_sig = backend.get_standard_root_signature();
        commands->SetComputeRootSignature(root_sig);

        auto* heap = renderer->get_render_backend().get_cbv_srv_uav_heap();
        commands->SetDescriptorHeaps(1, &heap);

        const auto array_descriptor = renderer->get_resource_array_gpu_descriptor(frame_idx);
        commands->SetComputeRootDescriptorTable(RenderBackend::RESOURCES_ARRAY_ROOT_PARAMETER_INDEX, array_descriptor);

        if(!fluid_volume_states.is_empty()) {
            record_fire_simulation_updates(commands, frame_idx);
        }

        // Always advance the arrays to the next frame so we can keep everything consistent
        advance_fire_sim_params_arrays();

        fluid_sim_dispatch_command_buffers.advance_frame();
    }

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
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "FluidSimPass::record_fire_simulation_updates");
        PIXScopedEvent(commands, PIX_COLOR(224, 96, 54), "record_fire_simulation_updates");

        set_buffer_indices(commands, frame_idx);

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

    void FluidSimPass::load_shaders() {
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

    void FluidSimPass::create_indirect_command_signature() {
        auto& backend = renderer->get_render_backend();

        // TODO: Measure if the order I set the root constants in has a measurable performance impact
        const auto arguments = Rx::
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
                                                            .DestOffsetIn32BitValues = RenderBackend::OBJECT_ID_ROOT_CONSTANT_OFFSET,
                                                            .Num32BitValuesToSet = 1}},
                  D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH}};

        const D3D12_COMMAND_SIGNATURE_DESC command_signature_desc{.ByteStride = 24,
                                                                  .NumArgumentDescs = static_cast<UINT>(arguments.size()),
                                                                  .pArgumentDescs = arguments.data()};
        const auto& root_sig = backend.get_standard_root_signature();
        backend.device->CreateCommandSignature(&command_signature_desc, root_sig, IID_PPV_ARGS(&fluid_sim_dispatch_signature));
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
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Advection");
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
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Bouyancy");
        PIXScopedEvent(commands, PIX_COLOR_DEFAULT, "Bouyancy");

        execute_simulation_step(commands, buoyancy_params_array, buoyancy_pipeline, [&](GpuFluidVolumeState& state, auto& barriers) {
            barrier_and_swap(state.velocity_textures, barriers);
        });
    }

    void FluidSimPass::apply_emitters(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Impulse");
        PIXScopedEvent(commands, PIX_COLOR_DEFAULT, "Impulse");

        execute_simulation_step(commands, emitters_params_array, emitters_pipeline, [&](GpuFluidVolumeState& state, auto& barriers) {
            barrier_and_swap(state.reaction_textures, barriers);
            barrier_and_swap(state.temperature_textures, barriers);
        });
    }

    void FluidSimPass::apply_extinguishment(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Extinguishment");
        PIXScopedEvent(commands, PIX_COLOR_DEFAULT, "Extinguishment");

        execute_simulation_step(commands,
                                extinguishment_params_array,
                                extinguishment_pipeline,
                                [&](GpuFluidVolumeState& state, auto& barriers) { barrier_and_swap(state.density_textures, barriers); });
    }

    void FluidSimPass::compute_vorticity_confinement(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Vorticity Confinement");
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
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Divergence");
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
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Pressure");
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
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Projection");
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
