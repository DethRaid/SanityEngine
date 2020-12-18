#include "single_pass_downsampler.hpp"

#include "loading/shader_loading.hpp"
#include "renderer/rhi/render_device.hpp"
#include "rhi/d3d12_private_data.hpp"
#include "rx/core/log.h"

#define A_CPU
#include "ffx_a.h"
#include "ffx_spd.h"

namespace sanity::engine::renderer {
    RX_LOG("SinglePassDenoiser", logger);

    SinglePassDenoiser SinglePassDenoiser::Create(RenderBackend& backend) {
        Rx::Vector<CD3DX12_ROOT_PARAMETER> spd_params;
        spd_params.resize(20);

        // Shader parameter constants
        spd_params[0].InitAsConstants(4, 0);

        // Texture definitions
        spd_params[1].InitAsUnorderedAccessView(3);
        spd_params[2].InitAsUnorderedAccessView(2);
        spd_params[3].InitAsConstantBufferView(0);

        const auto desc = D3D12_ROOT_SIGNATURE_DESC{.NumParameters = static_cast<UINT>(spd_params.size()),
                                                    .pParameters = spd_params.data()};

        const auto spd_root_sig = backend.compile_root_signature(desc);

        const auto compute_instructions = load_shader("utility/single_pass_downsampler.hlsl");

        const auto spd_pipeline = backend.create_compute_pipeline_state(compute_instructions, spd_root_sig);

        return SinglePassDenoiser{spd_root_sig, spd_pipeline};
    }

    void SinglePassDenoiser::generate_mip_chain_for_texture(ID3D12Resource* texture, ID3D12GraphicsCommandList* cmds) {
        varAU2(dispatch_thread_group_count_xy);
        varAU2(work_group_offset);
        varAU2(num_work_groups_and_mips);

        const auto desc = texture->GetDesc();
        varAU4(rectInfo) = initAU4(0, 0, static_cast<AU1>(desc.Width), desc.Height);

    	SpdSetup(dispatch_thread_group_count_xy, work_group_offset, num_work_groups_and_mips, rectInfo);
    	
        cmds->SetComputeRootSignature(root_signature.Get());
        cmds->SetPipelineState(pipeline.Get());

    	cmds->SetComputeRoot32BitConstant(0, num_work_groups_and_mips[1], MIP_COUNT_ROOT_CONSTANT_OFFSET);
        cmds->SetComputeRoot32BitConstant(0, num_work_groups_and_mips[0], NUM_WORK_GROUPS_ROOT_CONSTANT_OFFSET);
    	cmds->SetComputeRoot32BitConstants(0, 2, &work_group_offset, OFFSET_X_ROOT_CONSTANT_OFFSET);
            	
    	const auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(texture);
        cmds->ResourceBarrier(1, &barrier);
    	
    	cmds->Dispatch(dispatch_thread_group_count_xy[0], dispatch_thread_group_count_xy[1], 1);

        cmds->ResourceBarrier(1, &barrier);
    }

    SinglePassDenoiser::SinglePassDenoiser(ComPtr<ID3D12RootSignature> root_signature_in, ComPtr<ID3D12PipelineState> pipeline_in)
        : root_signature{Rx::Utility::move(root_signature_in)}, pipeline{Rx::Utility::move(pipeline_in)} {}
} // namespace sanity::engine::renderer
