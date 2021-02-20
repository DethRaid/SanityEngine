#include "fluid_sim_pass.hpp"

#include "core/components.hpp"
#include "entt/entity/registry.hpp"
#include "loading/shader_loading.hpp"
#include "renderer/render_components.hpp"
#include "renderer/renderer.hpp"
#include "renderer/rhi/d3d12_private_data.hpp"

namespace sanity::engine::renderer {
    FluidSimPass::FluidSimPass(Renderer& renderer_in) : renderer{&renderer_in} {
        auto& backend = renderer->get_render_backend();

        Rx::Vector<CD3DX12_ROOT_PARAMETER> fluid_sim_params;
        fluid_sim_params.resize(3);

        // Shader parameter constants
        fluid_sim_params[ROOT_CONSTANTS_INDEX].InitAsConstants(6, 0);

        // UAV table + global counter buffer
        fluid_sim_params[FLUID_SIM_PARAMS_BUFFER_INDEX].InitAsConstantBufferView(1);

        // Textures array
        Rx::Vector<D3D12_DESCRIPTOR_RANGE> descriptor_table_ranges;
        descriptor_table_ranges.push_back(D3D12_DESCRIPTOR_RANGE{
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            .NumDescriptors = MAX_NUM_TEXTURES,
            .BaseShaderRegister = 16,
            .RegisterSpace = 0,
            .OffsetInDescriptorsFromTableStart = 0,
        });

        fluid_sim_params[TEXTURES_ARRAY_INDEX].InitAsDescriptorTable(static_cast<UINT>(descriptor_table_ranges.size()),
                                                                     descriptor_table_ranges.data());

        auto static_sampler_desc = CD3DX12_STATIC_SAMPLER_DESC{0};
        static_sampler_desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        static_sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        static_sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        static_sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        static_sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        static_sampler_desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;

        const auto desc = D3D12_ROOT_SIGNATURE_DESC{.NumParameters = static_cast<UINT>(fluid_sim_params.size()),
                                                    .pParameters = fluid_sim_params.data(),
                                                    .NumStaticSamplers = 1,
                                                    .pStaticSamplers = &static_sampler_desc};

        fluid_sim_root_sig = backend.compile_root_signature(desc);
        set_object_name(fluid_sim_root_sig.Get(), "Fluid Sim Root Signature");

        const auto shader = load_shader("fluid_sim.compute");
        fluid_sim_pipeline = backend.create_compute_pipeline_state(shader, fluid_sim_root_sig);
        set_object_name(fluid_sim_pipeline.Get(), "Fluid Sim Pipeline");

        const auto arguments = Rx::Array{D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT,
                                                                      .Constant = {.RootParameterIndex = ROOT_CONSTANTS_INDEX,
                                                                                   .DestOffsetIn32BitValues = 0,
                                                                                   .Num32BitValuesToSet = 6}},
                                         D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH}};

        const D3D12_COMMAND_SIGNATURE_DESC command_signature_desc{.ByteStride = 0,
                                                                  .NumArgumentDescs = static_cast<UINT>(arguments.size()),
                                                                  .pArgumentDescs = arguments.data()};
        backend.device->CreateCommandSignature(&command_signature_desc,
                                               fluid_sim_root_sig.Get(),
                                               IID_PPV_ARGS(&fluid_sim_command_signature));
    }

    void FluidSimPass::render(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) {
        const auto& fluid_sims_view = registry.view<TransformComponent, FluidVolumeComponent>();

        auto& backend = renderer->get_render_backend();
        
        commands->SetComputeRootSignature(fluid_sim_root_sig.Get());
       
        commands->SetPipelineState(fluid_sim_pipeline.Get());

        fluid_sims_view.each(
            [&](const entt::entity& entity, const TransformComponent& transform, const FluidVolumeComponent& fluid_volume) {
                const auto model_matrix_index = renderer->add_model_matrix_to_frame(transform.get_model_matrix(registry), frame_idx);
                commands->SetComputeRoot32BitConstant(0, model_matrix_index, RenderBackend::MODEL_MATRIX_INDEX_ROOT_CONSTANT_OFFSET);

                commands->SetComputeRoot32BitConstant(0, fluid_volume.volume.index, RenderBackend::OBJECT_ID_ROOT_CONSTANT_OFFSET);

                commands->Dispatch(fluid_volume.volume_size.x, fluid_volume.volume_size.y, fluid_volume.volume_size.z);
            });
    }
} // namespace sanity::engine::renderer
