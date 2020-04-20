#pragma once

#include <string>

#include <d3d12.h>
#include <dxgi.h>

#include "../render_pipeline_state.hpp"
#include "../resources.hpp"

class ID3D12Object;

namespace rhi {
    constexpr uint64_t FENCE_UNSIGNALED = 0;
    constexpr uint64_t CPU_FENCE_SIGNALED = 32;
    constexpr uint64_t GPU_FENCE_SIGNALED = 64;
    constexpr uint32_t FRAME_COMPLETE = 128;

    std::wstring to_wide_string(const std::string& string);

    void set_object_name(ID3D12Object& object, const std::string& name);

    DXGI_FORMAT to_dxgi_format(ImageFormat format);

    D3D12_BLEND to_d3d12_blend(BlendFactor factor);

    D3D12_BLEND_OP to_d3d12_blend_op(BlendOp op);

    D3D12_FILL_MODE to_d3d12_fill_mode(FillMode mode);

    D3D12_CULL_MODE to_d3d12_cull_mode(CullMode mode);

    D3D12_COMPARISON_FUNC to_d3d12_comparison_func(CompareOp op);

    D3D12_STENCIL_OP to_d3d12_stencil_op(StencilOp op);

    D3D12_PRIMITIVE_TOPOLOGY_TYPE to_d3d12_primitive_topology_type(PrimitiveType topology);
} // namespace render
