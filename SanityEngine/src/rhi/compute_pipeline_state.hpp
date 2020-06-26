#pragma once

#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace renderer {
    /*!
     * \brief The state of a compute pipeline
     */
    struct ComputePipelineState {
        ComPtr<ID3D11ComputeShader> shader;
        ComPtr<ID3D12RootSignature> root_signature;
    };
} // namespace rhi
