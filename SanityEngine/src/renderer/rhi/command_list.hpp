#pragma once

#include <d3d12.h>
#include <winrt/base.h>

#include "core/types.hpp"

using winrt::com_ptr;

namespace renderer {
    struct CommandList {
        com_ptr<ID3D12GraphicsCommandList4> cmds;
        com_ptr<ID3D12CommandAllocator> command_allocator;
        Uint32 gpu_frame_idx;

        ID3D12GraphicsCommandList4* operator->() const;
    };

    inline ID3D12GraphicsCommandList4* CommandList::operator->() const { return cmds.get(); }
} // namespace renderer
