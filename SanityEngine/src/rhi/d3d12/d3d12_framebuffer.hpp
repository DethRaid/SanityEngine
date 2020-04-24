#pragma once

#include <optional>
#include <vector>

#include <d3d12.h>

#include "../framebuffer.hpp"

namespace rhi {
    struct D3D12Framebuffer : Framebuffer {
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtv_handles;
        std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> dsv_handle;

        float width;
        float height;
    };
} // namespace rhi
