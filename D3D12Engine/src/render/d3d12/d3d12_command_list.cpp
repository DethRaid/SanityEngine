#include "d3d12_command_list.hpp"

#include <rx/core/utility/move.h>

using rx::utility::move;

namespace render {
    D3D12CommandList::D3D12CommandList(rx::memory::allocator& allocator, ComPtr<ID3D12GraphicsCommandList> cmds)
        : internal_allocator{&allocator}, commands{move(cmds)}, most_recent_resource_states{allocator} {}

    D3D12CommandList::D3D12CommandList(D3D12CommandList&& old) noexcept
        : internal_allocator{old.internal_allocator}, completion_functions{move(old.completion_functions)} {
        old.internal_allocator = nullptr;
    }

    D3D12CommandList& D3D12CommandList::operator=(D3D12CommandList&& old) noexcept {
        internal_allocator = old.internal_allocator;
        completion_functions = move(old.completion_functions);

        old.internal_allocator = nullptr;

        return *this;
    }

    D3D12CommandList::~D3D12CommandList() {
        completion_functions.each_fwd([](const rx::function<void()>& func) { func(); });
    }

    void D3D12CommandList::add_completion_function(rx::function<void()> completion_func) {
        completion_functions.push_back(move(completion_func));
    }
} // namespace render
