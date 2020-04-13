#include "d3d12_command_list.hpp"

#ifdef interface
#undef interface
#endif

#include <rx/core/utility/move.h>
#include <rx/console/interface.h>
#include <rx/console/variable.h>

#include "../../core/cvar_names.hpp"


using rx::utility::move;

namespace render {
    D3D12CommandList::D3D12CommandList(rx::memory::allocator& allocator, ComPtr<ID3D12GraphicsCommandList> cmds)
        : internal_allocator{&allocator}, commands{move(cmds)}, most_recent_resource_states{allocator}, command_types{allocator} {
        if(const auto* enable_rhi_validation_var = rx::console::interface::find_variable_by_name(ENABLE_RHI_VALIDATION_NAME)) {
            should_do_validation = enable_rhi_validation_var->cast<bool>()->get();
        }
    }

    D3D12CommandList::D3D12CommandList(D3D12CommandList&& old) noexcept
        : internal_allocator{old.internal_allocator},
          completion_functions{move(old.completion_functions)},
          commands{move(old.commands)},
          most_recent_resource_states{move(old.most_recent_resource_states)},
          command_types{move(old.command_types)} {

        old.~D3D12CommandList();
    }

    D3D12CommandList& D3D12CommandList::operator=(D3D12CommandList&& old) noexcept {
        internal_allocator = old.internal_allocator;
        completion_functions = move(old.completion_functions);
        commands = move(old.commands);
        most_recent_resource_states = move(old.most_recent_resource_states);
        command_types = move(old.command_types);

        old.~D3D12CommandList();

        return *this;
    }

    D3D12CommandList::~D3D12CommandList() {
        completion_functions.each_fwd([](const rx::function<void()>& func) { func(); });
    }

    void D3D12CommandList::add_completion_function(rx::function<void()> completion_func) {
        completion_functions.push_back(move(completion_func));
    }

    const auto& D3D12CommandList::get_resource_states() const { return most_recent_resource_states; }

    const auto& D3D12CommandList::get_used_command_types() const { return command_types; }
} // namespace render
