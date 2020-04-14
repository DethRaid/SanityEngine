#pragma once

#include <d3d12.h>
#include <wrl/client.h>

#include "../render_pipeline_state.hpp"

using Microsoft::WRL::ComPtr;

namespace render {
    struct D3D12RenderPipelineState : RenderPipelineState {
        ComPtr<ID3D12PipelineState> pso;
        ComPtr<ID3D12RootSignature> root_signature;
    };
} // namespace render
