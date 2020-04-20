#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include "../compute_pipeline_state.hpp"

using Microsoft::WRL::ComPtr;

namespace rhi {
    struct D3D12ComputePipelineState : ComputePipelineState {
        ComPtr<ID3D12PipelineState> pso;
        ComPtr<ID3D12RootSignature> root_signature;
    };
} // namespace render
