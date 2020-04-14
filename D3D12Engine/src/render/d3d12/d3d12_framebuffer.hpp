#pragma once

#include <d3d12.h>
#include <rx/core/optional.h>
#include <rx/core/vector.h>

#include "../framebuffer.hpp"

namespace render {
    struct D3D12Framebuffer : Framebuffer {
        rx::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtv_handles;
        rx::optional<D3D12_CPU_DESCRIPTOR_HANDLE> dsv_handle;

        rx::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> render_target_descriptions;
        rx::optional<D3D12_RENDER_PASS_DEPTH_STENCIL_DESC> depth_stencil_desc;
    };
} // namespace render
