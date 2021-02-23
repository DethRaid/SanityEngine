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
                                                               .size = fluid_volume.size,
                                                               .dissipation = {fluid_volume.density_dissipation,
                                                                               fluid_volume.temperature_dissipation,
                                                                               1.0,
                                                                               fluid_volume.velocity_dissipation},
                                                               .decay = {0.f, 0.f, fluid_volume.reaction_decay, 0.f},
                                                               .buoyancy = fluid_volume.buoyancy,
                                                               .weight = fluid_volume.weight};
                fluid_volume_states.push_back(initial_state);

                set_resource_usage(density_textures[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                set_resource_usage(density_textures[1], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            });

        fluid_sim_dispatch_command_buffers.write(fluid_sim_dispatches);
    }

    void FluidSimPass::record_work(ID3D12GraphicsCommandList4* commands, entt::registry& registry, const Uint32 frame_idx) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "FluidSimPass::execute");

        auto& backend = renderer->get_render_backend();

        const auto& root_sig = backend.get_standard_root_signature();
        commands->SetComputeRootSignature(root_sig.Get());

        set_buffer_indices(commands, frame_idx);

        apply_advection(commands);

        apply_buoyancy(commands);

        apply_impulse(commands);

        // Assume that we only update fluid sims once per frame
        fluid_sim_dispatch_command_buffers.advance_frame();
        advection_params_array.advance_frame();
    }

    void FluidSimPass::load_shaders() {
        auto& backend = renderer->get_render_backend();

        const auto advection_shader = load_shader("fluid/advection.compute");
        advection_pipeline = backend.create_compute_pipeline_state(advection_shader);
        set_object_name(advection_pipeline.Get(), "Fluid Sim Advection");
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

        const D3D12_COMMAND_SIGNATURE_DESC command_signature_desc{.ByteStride = 0,
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
        }

        fluid_sim_dispatch_command_buffers.set_buffers(dispatch_command_buffers);

        advection_params_array.set_buffers(advection_params_buffers);
        buoyancy_params_array.set_buffers(buoyancy_params_buffers);
        impulse_params_array.set_buffers(impulse_params_buffers);
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
        const BufferHandle data_buffer_handle,
        ID3D12PipelineState* pipeline,
        const Rx::Function<void(GpuFluidVolumeState&, Rx::Vector<D3D12_RESOURCE_BARRIER>& barriers)> synchronize_volume) {
        commands->SetPipelineState(pipeline);

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

        commands->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    }

    void FluidSimPass::apply_advection(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Advection");

        advection_params_array.write(fluid_volume_states);
        const auto& data_buffer_handle = advection_params_array.get_active_buffer();

        execute_simulation_step(commands,
                                data_buffer_handle,
                                advection_pipeline.Get(),
                                [&](GpuFluidVolumeState& volume, Rx::Vector<D3D12_RESOURCE_BARRIER>& barriers) {
                                    barrier_and_swap(volume.density_textures, barriers);
                                    barrier_and_swap(volume.temperature_textures, barriers);
                                    barrier_and_swap(volume.reaction_textures, barriers);
                                    barrier_and_swap(volume.velocity_textures, barriers);
                                });
    }

    void FluidSimPass::apply_buoyancy(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Bouyancy");

        buoyancy_params_array.write(fluid_volume_states);
        const auto data_buffer_handle = buoyancy_params_array.get_active_buffer();

        execute_simulation_step(commands,
                                data_buffer_handle,
                                buoyancy_pipeline.Get(),
                                [&](GpuFluidVolumeState& state, Rx::Vector<D3D12_RESOURCE_BARRIER>& barriers) {
                                    barrier_and_swap(state.velocity_textures, barriers);
                                });
    }

    void FluidSimPass::apply_impulse(ID3D12GraphicsCommandList* commands) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "Impulse");

        impulse_params_array.write(fluid_volume_states);
        const auto data_buffer_handle = impulse_params_array.get_active_buffer();

        execute_simulation_step(commands, data_buffer_handle, impulse_pipeline.Get(), [&](auto& state, auto& barriers) {
            barrier_and_swap(state.reaction_textures, barriers);
            barrier_and_swap(state.temperature_textures, barriers);
        });
    }

    void FluidSimPass::barrier_and_swap(TextureHandle handles[2], Rx::Vector<D3D12_RESOURCE_BARRIER>& barriers) const {
        const auto& texture = renderer->get_texture(handles[1]);
        barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(texture.resource.Get()));

        Rx::Utility::swap(handles[0], handles[1]);
    }
} // namespace sanity::engine::renderer
