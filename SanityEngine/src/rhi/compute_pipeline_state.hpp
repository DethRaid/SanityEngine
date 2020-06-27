#pragma once

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace renderer {
    /*!
     * \brief The state of a compute pipeline
     */
    struct ComputePipelineState {
        ComPtr<ID3D12PipelineState> pso;
        ComPtr<ID3D12RootSignature> root_signature;
    };
} // namespace rhi
