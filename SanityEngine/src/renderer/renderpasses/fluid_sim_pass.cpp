#include "fluid_sim_pass.hpp"

#include "core/components.hpp"
#include "entt/entity/registry.hpp"
#include "loading/shader_loading.hpp"
#include "renderer/hlsl/fluid_sim.hpp"
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

        const auto arguments = Rx::
            Array{D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT,
                                               .Constant = {.RootParameterIndex = RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                                            .DestOffsetIn32BitValues = 0,
                                                            .Num32BitValuesToSet = 7}},
                  D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH}};

        const D3D12_COMMAND_SIGNATURE_DESC command_signature_desc{.ByteStride = 0,
                                                                  .NumArgumentDescs = static_cast<UINT>(arguments.size()),
                                                                  .pArgumentDescs = arguments.data()};
        const auto& root_sig = backend.get_standard_root_signature();
        backend.device->CreateCommandSignature(&command_signature_desc, root_sig.Get(), IID_PPV_ARGS(&fluid_sim_command_signature));
            	
        auto buffer_create_info = BufferCreateInfo{.usage = BufferUsage::ConstantBuffer, .size = sizeof(FluidVolume) * MAX_NUM_FLUID_VOLUMES};
        for(auto i = 0u; i < num_gpu_frames; i++) {
            const auto buffer_name = Rx::String::format("Fluid simulation data buffer %d", i);
            buffer_create_info.name = buffer_name;

        	const auto buffer = renderer->create_buffer(buffer_create_info);
            fluid_sim_data_buffers.push_back(buffer);
        }
    }

    void FluidSimPass::render(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) {

        const auto& fluid_sims_view = registry.view<TransformComponent, FluidVolumeComponent>();

        auto& backend = renderer->get_render_backend();

        const auto& root_sig = backend.get_standard_root_signature();
        commands->SetComputeRootSignature(root_sig.Get());

        commands->SetPipelineState(fluid_sim_pipeline.Get());

        fluid_sims_view.each(
            [&](const entt::entity& entity, const TransformComponent& transform, const FluidVolumeComponent& fluid_volume) {
                const auto model_matrix_index = renderer->add_model_matrix_to_frame(transform.get_model_matrix(registry), frame_idx);
                commands->SetComputeRoot32BitConstant(0, model_matrix_index, RenderBackend::MODEL_MATRIX_INDEX_ROOT_CONSTANT_OFFSET);

                commands->SetComputeRoot32BitConstant(0, static_cast<Uint32>(entity), RenderBackend::OBJECT_ID_ROOT_CONSTANT_OFFSET);
                commands->SetComputeRoot32BitConstant(0, fluid_volume.volume.index, RenderBackend::DATA_INDEX_ROOT_CONSTANT_OFFSET);

                commands->Dispatch(fluid_volume.volume->size.x, fluid_volume.volume->size.y, fluid_volume.volume->size.z);
            });
    }
} // namespace sanity::engine::renderer
