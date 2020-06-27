#pragma once

#include <d3d12.h>
#include <rx/core/types.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace renderer {
    struct CommandList {
        ComPtr<ID3D12GraphicsCommandList4> cmds;
        ComPtr<ID3D12CommandAllocator> command_allocator;
        Uint32 gpu_frame_idx;

        ID3D12GraphicsCommandList4* operator->() const;
    };

    inline ID3D12GraphicsCommandList4* CommandList::operator->() const {
        return cmds.Get();
    }
} // namespace renderer
