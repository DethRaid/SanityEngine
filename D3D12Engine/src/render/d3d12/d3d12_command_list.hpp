#pragma once

#include <set>

#include <d3d12.h>
#include <wrl/client.h>

#include "../command_list.hpp"

using Microsoft::WRL::ComPtr;

namespace render {
    struct D3D12Buffer;
    struct D3D12Image;

    /*!
     * \brief Base class for D3D12 command lists
     *
     * The D3D12 command list executes the completion functions in its destructor. Thus, the render device that executes a D3D12CommandList
     * should take care to not destruct it until it has finished executing on the GPU
     */
    class D3D12CommandList : virtual public CommandList {
    public:
        explicit D3D12CommandList(const ComPtr<ID3D12GraphicsCommandList>& cmds);

        D3D12CommandList(const D3D12CommandList& other) = delete;
        D3D12CommandList& operator=(const D3D12CommandList& other) = delete;

        D3D12CommandList(D3D12CommandList&& old) noexcept;
        D3D12CommandList& operator=(D3D12CommandList&& old) noexcept;

#pragma region CommandList
        ~D3D12CommandList() override = default;

        void add_completion_function(std::function<void()>&& completion_func) override;
#pragma endregion

        [[nodiscard]] const auto& get_final_resource_states() const;

        [[nodiscard]] const auto& get_used_command_types() const;

        [[nodiscard]] ID3D12CommandList* get_command_list() const;

        void execute_completion_functions();

    protected:
        std::vector<std::function<void()>> completion_functions;

        ComPtr<ID3D12GraphicsCommandList> commands;

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
        void set_resource_state(const D3D12Image& image, D3D12_RESOURCE_STATES new_states);

        /*!
         * \brief Updates the resource state tracking for the provided resource, recording a barrier to transition resource state if needed
         */
        void set_resource_state(const D3D12Buffer& buffer, D3D12_RESOURCE_STATES new_states);

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
} // namespace render
