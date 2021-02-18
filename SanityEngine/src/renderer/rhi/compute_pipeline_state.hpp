#pragma once

#include <d3d12.h>
#include <wrl/client.h>

namespace sanity::engine::renderer {
    using Microsoft::WRL::ComPtr;

    struct ComputePipelineState {
        ComPtr<ID3D12PipelineState> pso;
        ComPtr<ID3D12RootSignature> root_signature;
    };
} // namespace sanity::engine::renderer
