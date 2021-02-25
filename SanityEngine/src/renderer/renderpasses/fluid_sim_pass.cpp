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

    FluidSimPass::FluidSimPass(Renderer& renderer_in) : renderer{&renderer_in} {
        ZoneScoped;

        auto& backend = renderer->get_render_backend();
        const auto num_gpu_frames = backend.get_max_num_gpu_frames();

        load_shaders();

        create_indirect_command_signature(backend);

        create_buffers(num_gpu_frames);
    }

    void FluidSimPass::collect_work(entt::registry& registry, const Uint32 frame_idx) {
        ZoneScoped;

        fluid_sim_dispatches.clear();

        const auto& fluid_sims_view = registry.view<TransformComponent, FluidVolumeComponent>();
        fluid_sim_dispatches.reserve(fluid_sims_view.size());

        fluid_sims_view.each(
            [&](const entt::entity& entity, const TransformComponent& transform, const FluidVolumeComponent& fluid_volume_component) {
                const auto model_matrix_index = renderer->add_model_matrix_to_frame(transform.get_model_matrix(registry), frame_idx);

                const auto& fluid_volume = renderer->get_fluid_volume(fluid_volume_component.volume);

                const auto& volume_size = fluid_volume.size;
                const auto dispatch = FluidSimDispatch{.fluid_volume_idx = fluid_volume_component.volume.index,
                                                       .entity_id = static_cast<uint>(entity),
                                                       .model_matrix_idx = model_matrix_index,
                                                       .thread_group_count_x = volume_size.x / FLUID_SIM_NUM_THREADS,
                                                       .thread_group_count_y = volume_size.y / FLUID_SIM_NUM_THREADS,
                                                       .thread_group_count_z = volume_size.z / FLUID_SIM_NUM_THREADS};
                fluid_sim_dispatches.push_back(dispatch);

                const auto& density_textures = fluid_volume.density_texture[frame_idx];
                const auto& temperature_textures = fluid_volume.temperature_texture[frame_idx];
                const auto& reaction_textures = fluid_volume.reaction_texture[frame_idx];
                const auto& velocity_textures = fluid_volume.velocity_texture[frame_idx];
                const auto& pressure_textures = fluid_volume.velocity_texture[frame_idx];

                const auto initial_state = GpuFluidVolumeState{.density_textures = {density_textures[0], density_textures[1]},
                                                               .temperature_textures = {temperature_textures[0], temperature_textures[1]},
                                                               .reaction_textures = {reaction_textures[0], reaction_textures[1]},
                                                               .velocity_textures = {velocity_textures[0], velocity_textures[1]},
                                                               .pressure_textures = {pressure_textures[0], pressure_textures[1]},
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
                .emitter_strength = fluid_volume.emitter_strength};
                fluid_volume_states.push_back(initial_state);

                set_resource_usage(density_textures[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                set_resource_usage(density_textures[1], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            });

        renderer->copy_data_to_buffer(fluid_sim_dispatch_command_buffers.get_active_buffer(), fluid_sim_dispatches);
    }

    void FluidSimPass::record_work(ID3D12GraphicsCommandList4* commands, entt::registry& registry, const Uint32 frame_idx) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "FluidSimPass::execute");

        auto& backend = renderer->get_render_backend();

        const auto& root_sig = backend.get_standard_root_signature();
        commands->SetComputeRootSignature(root_sig.Get());

        record_fire_simulation_updates(commands, frame_idx);

        // Assume that we only update fluid sims once per frame

        advance_fire_sim_params_arrays();

        fluid_sim_dispatch_command_buffers.advance_frame();
    }

    void FluidSimPass::record_fire_simulation_updates(ID3D12GraphicsCommandList4* commands, const Uint32 frame_idx) {
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
        // volume of the fluid does not change over time. The divergence is the amount the field has deviated from being divergence free [you mean compression free?]
        compute_divergence(commands);

        // This computes the pressure needed to return the fluid to a divergence free condition
        compute_pressure(commands);

        // Subtract the pressure field from the velocity field enforcing the divergence free conditions
        compute_projection(commands);
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
        set_object_name(advection_pipeline.Get(), "Fluid Sim Advection");

        const auto buoyancy_shader = load_shader("fluid/apply_buoyancy.compute");
        buoyancy_pipeline = backend.create_compute_pipeline_state(buoyancy_shader);
        set_object_name(buoyancy_pipeline.Get(), "Fluid Sim Buoyancy");

        const auto emitters_shader = load_shader("fluid/apply_emitters.compute");
        emitters_pipeline = backend.create_compute_pipeline_state(emitters_shader);
        set_object_name(emitters_pipeline.Get(), "Fluid Sim Emitters");
        
        const auto extinguishment_shader = load_shader("fluid/apply_extinguishment.compute");
        extinguishment_pipeline = backend.create_compute_pipeline_state(extinguishment_shader);
        set_object_name(extinguishment_pipeline.Get(), "Fluid Sim Extinguishment");
    	
        // const auto vorticity_shader = load_shader("fluid/compute_vorticity.compute");
        // vorticity_pipeline = backend.create_compute_pipeline_state(vorticity_shader);
        // set_object_name(vorticity_pipeline.Get(), "Fluid Sim Vorticity");
        //     	
        // const auto confinement_shader = load_shader("fluid/compute_confinement.compute");
        // confinement_pipeline = backend.create_compute_pipeline_state(confinement_shader);
        // set_object_name(confinement_pipeline.Get(), "Fluid Sim Confinement");
        //     	
        // const auto divergence_shader = load_shader("fluid/compute_divergence.compute");
        // divergence_pipeline = backend.create_compute_pipeline_state(divergence_shader);
        // set_object_name(divergence_pipeline.Get(), "Fluid Sim Advection");
    	// 
        // const auto pressure_shader = load_shader("fluid/jacobi_pressure_solver.compute");
        // jacobi_pressure_solver_pipeline = backend.create_compute_pipeline_state(pressure_shader);
        // set_object_name(jacobi_pressure_solver_pipeline.Get(), "Fluid Sim Advection");
    	// 
        // const auto projection_shader = load_shader("fluid/compute_projection.compute");
        // projection_pipeline = backend.create_compute_pipeline_state(projection_shader);
        // set_object_name(projection_pipeline.Get(), "Fluid Sim Advection");
    }

    void FluidSimPass::create_indirect_command_signature(RenderBackend& backend) {
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
        backend.device->CreateCommandSignature(&command_signature_desc, root_sig.Get(), IID_PPV_ARGS(&fluid_sim_dispatch_signature));
    }

    void FluidSimPass::create_buffers(const Uint32 num_gpu_frames) {
        ZoneScoped;

        Rx::Vector<BufferHandle> dispatch_command_buffers{num_gpu_frames};

        Rx::Vector<BufferHandle> advection_params_buffers{num_gpu_frames};
        Rx::Vector<BufferHandle> buoyancy_params_buffers{num_gpu_frames};
        Rx::Vector<BufferHandle> impulse_params_buffers{num_gpu_frames};
        Rx::Vector<BufferHandle> extinguishment_params_buffers{num_gpu_frames};

        const auto fluid_sim_state_buffer_size = MAX_NUM_FLUID_VOLUMES * sizeof(GpuFluidVolumeState);

        for(auto i = 0u; i < num_gpu_frames; i++) {
            const auto dispatch_buffer_name = Rx::String::format("Fluid Sim Dispatch Commands buffer %d", i);
            const auto dispatch_buffer_create_info = BufferCreateInfo{.name = dispatch_buffer_name,
                                                                      .usage = BufferUsage::ConstantBuffer,
                                                                      .size = MAX_NUM_FLUID_VOLUMES * sizeof(FluidSimDispatch)};
            dispatch_command_buffers[i] = renderer->create_buffer(dispatch_buffer_create_info);

            const auto advection_buffer_name = Rx::String::format("Advection Params Array buffer %d", i);
            const auto advection_buffer_create_info = BufferCreateInfo{.name = advection_buffer_name,
                                                                       .usage = BufferUsage::ConstantBuffer,
                                                                       .size = fluid_sim_state_buffer_size};
            advection_params_buffers[i] = renderer->create_buffer(advection_buffer_create_info);

            const auto buoyancy_buffer_name = Rx::String::format("Buoyancy Params Array buffer %d", i);
            const auto buoyancy_buffer_create_info = BufferCreateInfo{.name = buoyancy_buffer_name,
                                                                      .usage = BufferUsage::ConstantBuffer,
                                                                      .size = fluid_sim_state_buffer_size};
            buoyancy_params_buffers[i] = renderer->create_buffer(buoyancy_buffer_create_info);

            const auto impulse_buffer_name = Rx::String::format("Impulse Params Array buffer %d", i);
            const auto impulse_buffer_create_info = BufferCreateInfo{.name = impulse_buffer_name,
                                                                     .usage = BufferUsage::ConstantBuffer,
                                                                     .size = fluid_sim_state_buffer_size};
            impulse_params_buffers[i] = renderer->create_buffer(impulse_buffer_create_info);

            const auto extinguishment_buffer_name = Rx::String::format("Extinguishment Params Array buffer %d", i);
            const auto extinguishment_buffer_create_info = BufferCreateInfo{.name = extinguishment_buffer_name,
                                                                            .usage = BufferUsage::ConstantBuffer,
                                                                            .size = fluid_sim_state_buffer_size};
            extinguishment_params_buffers[i] = renderer->create_buffer(extinguishment_buffer_create_info);
        }

        fluid_sim_dispatch_command_buffers.set_buffers(dispatch_command_buffers);

        advection_params_array.set_buffers(advection_params_buffers);
        buoyancy_params_array.set_buffers(buoyancy_params_buffers);
        emitters_params_array.set_buffers(impulse_params_buffers);
        extinguishment_params_array.set_buffers(extinguishment_params_buffers);
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
        const PerFrameBuffer& data_buffer,
        const ComPtr<ID3D12PipelineState>& pipeline,
        const Rx::Function<void(GpuFluidVolumeState&, Rx::Vector<D3D12_RESOURCE_BARRIER>& barriers)> synchronize_volume) {
        const auto data_buffer_handle = data_buffer.get_active_buffer();
        renderer->copy_data_to_buffer(data_buffer_handle, fluid_volume_states);

        commands->SetPipelineState(pipeline.Get());

        commands->SetComputeRoot32BitConstant(RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                              data_buffer_handle.index,
                                              RenderBackend::DATA_BUFFER_INDEX_ROOT_PARAMETER_OFFSET);

        const auto& dispatch_buffer_handle = fluid_sim_dispatch_command_buffers.get_active_buffer();
        const auto& dispatch_buffer = renderer->get_buffer(dispatch_buffer_handle);
        commands->ExecuteIndirect(fluid_sim_dispatch_signature.Get(),
                                  static_cast<UINT>(fluid_sim_dispatches.size()),
                                  dispatch_buffer->resource.Get(),
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

        execute_simulation_step(commands, advection_params_array, advection_pipeline, [&](auto& volume, auto& barriers) {
            barrier_and_swap(volume.density_textures, barriers);
            barrier_and_swap(volume.temperature_textures, barriers);
            barrier_and_swap(volume.reaction_textures, barriers);
            barrier_and_swap(volume.velocity_textures, barriers);
        });
    }

    void FluidSimPass::apply_buoyancy(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Bouyancy");

        execute_simulation_step(commands, buoyancy_params_array, buoyancy_pipeline, [&](auto& state, auto& barriers) {
            barrier_and_swap(state.velocity_textures, barriers);
        });
    }

    void FluidSimPass::apply_emitters(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Impulse");

        execute_simulation_step(commands, emitters_params_array, emitters_pipeline, [&](auto& state, auto& barriers) {
            barrier_and_swap(state.reaction_textures, barriers);
            barrier_and_swap(state.temperature_textures, barriers);
        });
    }

    void FluidSimPass::apply_extinguishment(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Extinguishment");

        execute_simulation_step(commands, extinguishment_params_array, extinguishment_pipeline, [&](auto& state, auto& barriers) {
            barrier_and_swap(state.density_textures, barriers);
        });
    }

    void FluidSimPass::compute_vorticity_confinement(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Vorticity Confinement");

        execute_simulation_step(commands, vorticity_confinement_params_array, vorticity_pipeline, [&](auto& state, auto& barriers) {
            const auto& temp_data_texture = renderer->get_texture(state.temp_data_buffer);
            barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(temp_data_texture.resource.Get()));
        });

        execute_simulation_step(commands, vorticity_confinement_params_array, confinement_pipeline, [&](auto& state, auto& barriers) {
            barrier_and_swap(state.velocity_textures, barriers);
        });
    }

    void FluidSimPass::compute_divergence(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Divergence");

        execute_simulation_step(commands, divergence_params_array, divergence_pipeline, [&](auto& state, auto& barriers) {
            const auto& temp_data_texture = renderer->get_texture(state.temp_data_buffer);
            barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(temp_data_texture.resource.Get()));
        });
    }

    void FluidSimPass::compute_pressure(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Pressure");

        for(auto i = 0; i < *num_pressure_iterations; i++) {
            execute_simulation_step(commands, pressure_param_arrays[i], jacobi_pressure_solver_pipeline, [&](auto& state, auto& barriers) {
                barrier_and_swap(state.pressure_textures, barriers);
            });
        }
    }

    void FluidSimPass::compute_projection(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Projection");

        execute_simulation_step(commands, projection_param_arrays, projection_pipeline, [&](auto& state, auto& barriers) {
            barrier_and_swap(state.velocity_textures, barriers);
        });
    }

    void FluidSimPass::barrier_and_swap(TextureHandle handles[2], Rx::Vector<D3D12_RESOURCE_BARRIER>& barriers) const {
    	const auto& old_read_texture = renderer->get_texture(handles[0]);
        const auto& old_write_texture = renderer->get_texture(handles[1]);

    	barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(old_read_texture.resource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(old_write_texture.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
   	
        Rx::Utility::swap(handles[0], handles[1]);
    }
} // namespace sanity::engine::renderer
