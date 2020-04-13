#pragma once

#define interface struct

#include <d3d12.h>
#include <rx/core/map.h>
#include <rx/core/set.h>
#include <rx/core/vector.h>

#define interface struct

#include <wrl/client.h>

#include "../command_list.hpp"

using Microsoft::WRL::ComPtr;

namespace render {
    /*!
     * \brief Base class for D3D12 command lists
     *
     * The D3D12 command list executes the completion functions in its destructor. Thus, the render device that executes a D3D12CommandList
     * should take care to not destruct it until it has finished executing on the GPU
     */
    class D3D12CommandList : virtual public CommandList {
    public:
        D3D12CommandList(rx::memory::allocator& allocator, ComPtr<ID3D12GraphicsCommandList> cmds);

        D3D12CommandList(const D3D12CommandList& other) = delete;
        D3D12CommandList& operator=(const D3D12CommandList& other) = delete;

        D3D12CommandList(D3D12CommandList&& old) noexcept;
        D3D12CommandList& operator=(D3D12CommandList&& old) noexcept;

#pragma region CommandList
        ~D3D12CommandList() override;

        void add_completion_function(rx::function<void()> completion_func) override;
#pragma endregion

        [[nodiscard]] const auto& get_resource_states() const;

        [[nodiscard]] const auto& get_used_command_types() const;

    protected:
        rx::memory::allocator* internal_allocator;

        rx::vector<rx::function<void()>> completion_functions;

        ComPtr<ID3D12GraphicsCommandList> commands;

        rx::map<ID3D12Resource*, D3D12_RESOURCE_STATES> most_recent_resource_states;

        /*!
         * \brief Keeps track of all the types of commands that this command list uses
         */
        rx::set<D3D12_COMMAND_LIST_TYPE> command_types;

        bool should_do_validation = false;
    };
} // namespace render