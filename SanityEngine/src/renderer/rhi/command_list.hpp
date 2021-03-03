#pragma once

#include <d3d12.h>

#include "core/types.hpp"

namespace renderer {
    struct CommandList {
        sanity::engine::ComPtr<ID3D12GraphicsCommandList4> cmds;
        sanity::engine::ComPtr<ID3D12CommandAllocator> command_allocator;
        Uint32 gpu_frame_idx;

        ID3D12GraphicsCommandList4* operator->() const;
    };

    inline ID3D12GraphicsCommandList4* CommandList::operator->() const { return cmds; }
} // namespace renderer
