#include "d3d12_command_list.hpp"

#include "d3dx12.hpp"
#include "resources.hpp"

using std::move;

namespace render {
    D3D12CommandList::D3D12CommandList(const ComPtr<ID3D12GraphicsCommandList>& cmds) : commands{cmds} {}

    D3D12CommandList::D3D12CommandList(D3D12CommandList&& old) noexcept
        : completion_functions{move(old.completion_functions)},
          commands{move(old.commands)},
          most_recent_resource_states{move(old.most_recent_resource_states)},
          command_types{move(old.command_types)} {}

    D3D12CommandList& D3D12CommandList::operator=(D3D12CommandList&& old) noexcept {

        completion_functions = move(old.completion_functions);
        commands = move(old.commands);
        most_recent_resource_states = move(old.most_recent_resource_states);
        command_types = move(old.command_types);

        return *this;
    }

    void D3D12CommandList::add_completion_function(std::function<void()>&& completion_func) {
        completion_functions.push_back(std::move(completion_func));
    }

    void D3D12CommandList::prepare_for_submission() {
        commands->Close();
    }

    ID3D12CommandList* D3D12CommandList::get_command_list() const { return commands.Get(); }

    void D3D12CommandList::execute_completion_functions() {
        for(const auto& func : completion_functions) {
            func();
        }
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
        if(const auto& resource_states = most_recent_resource_states.find(resource); resource_states != most_recent_resource_states.end()) {
            if(need_barrier_between_states(resource_states->second, new_states, is_buffer_or_simultaneous_access_texture)) {
                const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, resource_states->second, new_states);
                commands->ResourceBarrier(1, &barrier);
            }

            resource_states->second = new_states;

        } else {
            initial_resource_states.emplace(resource, new_states);
            most_recent_resource_states.emplace(resource, new_states);
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
