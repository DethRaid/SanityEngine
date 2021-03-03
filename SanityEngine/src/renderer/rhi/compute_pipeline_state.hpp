#pragma once

#include <d3d12.h>

#include "core/types.hpp"

namespace sanity::engine::renderer {
    struct ComputePipelineState {
        ComPtr<ID3D12PipelineState> pso;
        ComPtr<ID3D12RootSignature> root_signature;
    };
} // namespace sanity::engine::renderer
