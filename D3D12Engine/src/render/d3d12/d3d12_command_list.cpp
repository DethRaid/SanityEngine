#include "d3d12_command_list.hpp"

#ifdef interface
#undef interface
#endif

#include <rx/console/interface.h>
#include <rx/console/variable.h>
#include <rx/core/utility/move.h>

#include "../../core/cvar_names.hpp"
#include "d3dx12.hpp"
#include "resources.hpp"

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
    }

    D3D12CommandList& D3D12CommandList::operator=(D3D12CommandList&& old) noexcept {
        internal_allocator = old.internal_allocator;
        completion_functions = move(old.completion_functions);
        commands = move(old.commands);
        most_recent_resource_states = move(old.most_recent_resource_states);
        command_types = move(old.command_types);

        return *this;
    }

    D3D12CommandList::~D3D12CommandList() {
        completion_functions.each_fwd([](const rx::function<void()>& func) { func(); });
    }

    void D3D12CommandList::add_completion_function(rx::function<void()> completion_func) {
        completion_functions.push_back(move(completion_func));
    }

    void D3D12CommandList::set_resource_state(const D3D12Image& image, const D3D12_RESOURCE_STATES new_states) {
        set_resource_state(image.resource.Get(), new_states, false);
    }

    void D3D12CommandList::set_resource_state(const D3D12Buffer& buffer, const D3D12_RESOURCE_STATES new_states) {
        set_resource_state(buffer.resource.Get(), new_states, true);
    }

    void D3D12CommandList::set_resource_state(ID3D12Resource* resource,
                                              const D3D12_RESOURCE_STATES new_states,
                                              const bool is_buffer_or_simultaneous_access_texture) {
        if(auto* resource_states = most_recent_resource_states.find(resource)) {
            if(need_barrier_between_states(*resource_states, new_states, is_buffer_or_simultaneous_access_texture)) {
                const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, *resource_states, new_states);
                commands->ResourceBarrier(1, &barrier);
            }

            *resource_states = new_states;

        } else {
            initial_resource_states.insert(resource, new_states);
            most_recent_resource_states.insert(resource, new_states);
        }
    }

    bool D3D12CommandList::need_barrier_between_states(const D3D12_RESOURCE_STATES old_states,
                                                       const D3D12_RESOURCE_STATES new_states,
                                                       const bool is_buffer_or_simultaneous_access_texture) {
        if(old_states == new_states) {
            // No need to transition if the states are the same
            return false;
        }

        if(old_states == D3D12_RESOURCE_STATE_COMMON) {
            if((new_states & D3D12_RESOURCE_STATE_DEPTH_READ) != 0 || (new_states & D3D12_RESOURCE_STATE_DEPTH_WRITE) != 0) {
                return true;
            }

            if(is_buffer_or_simultaneous_access_texture) {
                return false;
            }

            if(new_states == D3D12_RESOURCE_STATE_COPY_DEST || new_states == D3D12_RESOURCE_STATE_COPY_SOURCE ||
               new_states == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE || new_states == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
                return true;
            }
        }

        return true;
    }

    const auto& D3D12CommandList::get_final_resource_states() const { return most_recent_resource_states; }

    const auto& D3D12CommandList::get_used_command_types() const { return command_types; }
} // namespace render
