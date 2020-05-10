#include "command_list.hpp"

#include <utility>

#include <spdlog/sinks/stdout_color_sinks.h>

#include "d3dx12.hpp"
#include "helpers.hpp"
#include "minitrace.h"
#include "resources.hpp"

namespace rhi {
    CommandList::CommandList(ComPtr<ID3D12GraphicsCommandList4> cmds) : commands{std::move(cmds)} {}

    CommandList::CommandList(CommandList&& old) noexcept
        : completion_functions{std::move(old.completion_functions)},
          commands{std::move(old.commands)},
          most_recent_resource_states{std::move(old.most_recent_resource_states)},
          command_types{std::move(old.command_types)} {}

    CommandList& CommandList::operator=(CommandList&& old) noexcept {

        completion_functions = std::move(old.completion_functions);
        commands = std::move(old.commands);
        most_recent_resource_states = std::move(old.most_recent_resource_states);
        command_types = std::move(old.command_types);

        return *this;
    }

    void CommandList::set_debug_name(const std::string& name) const { set_object_name(*commands.Get(), name); }

    void CommandList::add_completion_function(std::function<void()>&& completion_func) {
        MTR_SCOPE("CommandList", "add_completion_function");
        completion_functions.push_back(std::move(completion_func));
    }

    void CommandList::prepare_for_submission() {
        MTR_SCOPE("CommandList", "prepare_for_submission");
        commands->Close();
    }

    ID3D12GraphicsCommandList4* CommandList::operator->() const { return commands.Get(); }

    ID3D12GraphicsCommandList4* CommandList::get_command_list() const { return commands.Get(); }

    void CommandList::execute_completion_functions() {
        MTR_SCOPE("CommandList", "execute_completion_functions");
        for(const auto& func : completion_functions) {
            func();
        }
    }

    void CommandList::set_resource_state(const Image& image, const D3D12_RESOURCE_STATES new_states) {
        MTR_SCOPE("CommandList", "set_resource_state(D3D12Image)");
        set_resource_state(image.resource.Get(), new_states, false);
    }

    void CommandList::set_resource_state(const Buffer& buffer, const D3D12_RESOURCE_STATES new_states) {
        MTR_SCOPE("CommandList", "set_resource_state(D3D12Buffer)");
        set_resource_state(buffer.resource.Get(), new_states, true);
    }

    void CommandList::set_resource_state(ID3D12Resource* resource,
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

    bool CommandList::need_barrier_between_states(const D3D12_RESOURCE_STATES old_states,
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

    const auto& CommandList::get_final_resource_states() const { return most_recent_resource_states; }

    const auto& CommandList::get_used_command_types() const { return command_types; }
} // namespace rhi
