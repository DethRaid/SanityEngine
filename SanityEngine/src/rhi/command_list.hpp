#pragma once

#include <functional>
#include <set>

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace rhi {
    struct Buffer;
    struct Image;

    /*!
     * \brief Generic implementation of a command list
     */
    class CommandList {
    public:
        explicit CommandList(ComPtr<ID3D12GraphicsCommandList4> cmds);

        CommandList(const CommandList& other) = delete;
        CommandList& operator=(const CommandList& other) = delete;

        CommandList(CommandList&& old) noexcept;
        CommandList& operator=(CommandList&& old) noexcept;

        void set_debug_name(const std::string& name) const;

        void add_completion_function(std::function<void()>&& completion_func);

        /*!
         * \brief Preforms all the necessary tasks to prepare this command list for submission to the GPU
         */
        virtual void prepare_for_submission();

        [[nodiscard]] const auto& get_final_resource_states() const;

        [[nodiscard]] const auto& get_used_command_types() const;

        [[nodiscard]] ID3D12GraphicsCommandList4* get_command_list() const;

        void execute_completion_functions();

    protected:
        std::vector<std::function<void()>> completion_functions;

        ComPtr<ID3D12GraphicsCommandList4> commands;

        std::unordered_map<ID3D12Resource*, D3D12_RESOURCE_STATES> initial_resource_states;
        std::unordered_map<ID3D12Resource*, D3D12_RESOURCE_STATES> most_recent_resource_states;

        /*!
         * \brief Keeps track of all the types of commands that this command list uses
         */
        std::set<D3D12_COMMAND_LIST_TYPE> command_types;

        bool should_do_validation = false;

        /*!
         * \brief Updates the resource state tracking for the provided resource, recording a barrier to transition resource state if needed
         */
        void set_resource_state(const Image& image, D3D12_RESOURCE_STATES new_states);

        /*!
         * \brief Updates the resource state tracking for the provided resource, recording a barrier to transition resource state if needed
         */
        void set_resource_state(const Buffer& buffer, D3D12_RESOURCE_STATES new_states);

        /*!
         * \brief Updates the resource state tracking for the provided resource, recording a barrier to transition resource state if needed
         */
        void set_resource_state(ID3D12Resource* resource, D3D12_RESOURCE_STATES new_states, bool is_buffer_or_simultaneous_access_texture);

        /*!
         * \brief Checks if we need a barrier between the old and new resource states
         */
        bool need_barrier_between_states(D3D12_RESOURCE_STATES old_states,
                                         D3D12_RESOURCE_STATES new_states,
                                         bool is_buffer_or_simultaneous_access_texture);
    };
} // namespace rhi
