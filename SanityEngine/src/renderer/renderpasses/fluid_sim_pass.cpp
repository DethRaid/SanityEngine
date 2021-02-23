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
        auto& backend = renderer->get_render_backend();
        const auto num_gpu_frames = backend.get_max_num_gpu_frames();

        const auto shader = load_shader("fluid_sim.compute");
        fluid_sim_pipeline = backend.create_compute_pipeline_state(shader);
        set_object_name(fluid_sim_pipeline.Get(), "Fluid Sim Pipeline");

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
        backend.device->CreateCommandSignature(&command_signature_desc, root_sig.Get(), IID_PPV_ARGS(&fluid_sim_command_signature));
    }

    void FluidSimPass::collect_work(entt::registry& registry, const Uint32 frame_idx) {
        ZoneScoped;

        fluid_sim_dispatches.clear();

        const auto& fluid_sims_view = registry.view<TransformComponent, FluidVolumeComponent>();
        fluid_sim_dispatches.reserve(fluid_sims_view.size());

        fluid_sims_view.each(
            [&](const entt::entity& entity, const TransformComponent& transform, const FluidVolumeComponent& fluid_volume) {
                const auto model_matrix_index = renderer->add_model_matrix_to_frame(transform.get_model_matrix(registry), frame_idx);
                const auto& volume_size = fluid_volume.volume->size;
                const auto dispatch = FluidSimDispatch{.fluid_volume_idx = fluid_volume.volume.index,
                                                       .entity_id = static_cast<uint>(entity),
                                                       .model_matrix_idx = model_matrix_index,
                                                       .thread_group_count_x = volume_size.x,
                                                       .thread_group_count_y = volume_size.y,
                                                       .thread_group_count_z = volume_size.z};
                fluid_sim_dispatches.push_back(dispatch);

            	set_resource_usage(fluid_volume.volume->texture_1_handle,
                                   D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            });

        const auto& dispatch_buffer = fluid_sim_dispatch_command_buffers[frame_idx];
        memcpy(dispatch_buffer->mapped_ptr, fluid_sim_dispatches.data(), fluid_sim_dispatches.size() * sizeof(FluidSimDispatch));
    }

    void FluidSimPass::record_work(ID3D12GraphicsCommandList4* commands, entt::registry& registry, const Uint32 frame_idx) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "FluidSimPass::execute");

        auto& backend = renderer->get_render_backend();

        const auto& root_sig = backend.get_standard_root_signature();
        commands->SetComputeRootSignature(root_sig.Get());

        const auto& frame_constants_buffer = renderer->get_frame_constants_buffer(frame_idx);
        commands->SetComputeRoot32BitConstant(RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                              frame_constants_buffer.index,
                                              RenderBackend::FRAME_CONSTANTS_BUFFER_INDEX_ROOT_CONSTANT_OFFSET);

        const auto& fluid_sims_buffer = renderer->get_fluid_volumes_buffer(frame_idx);
        commands->SetComputeRoot32BitConstant(RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                              fluid_sims_buffer.index,
                                              RenderBackend::DATA_BUFFER_INDEX_ROOT_PARAMETER_OFFSET);

        const auto& model_matrix_buffer = renderer->get_model_matrix_for_frame(frame_idx);
        commands->SetComputeRoot32BitConstant(RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                              model_matrix_buffer.index,
                                              RenderBackend::MODEL_MATRIX_BUFFER_INDEX_ROOT_CONSTANT_OFFSET);

        commands->SetPipelineState(fluid_sim_pipeline.Get());

        {
            TracyD3D12Zone(RenderBackend::tracy_context, commands, "FluidSimPass::ExecuteIndirect");
        	
            const auto& dispatch_buffer = fluid_sim_dispatch_command_buffers[frame_idx];
            commands->ExecuteIndirect(fluid_sim_command_signature.Get(),
                                      static_cast<UINT>(fluid_sim_dispatches.size()),
                                      dispatch_buffer->resource.Get(),
                                      0,
                                      nullptr,
                                      0);
        }
    }
} // namespace sanity::engine::renderer
