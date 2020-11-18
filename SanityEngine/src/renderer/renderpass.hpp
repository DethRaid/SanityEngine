#pragma once

#include <d3d12.h>

#include "core/types.hpp"
#include "entt/entity/fwd.hpp"
#include "renderer/handles.hpp"
#include "rx/core/map.h"

class World;

namespace renderer {
    /*!
     * \brief Tuple of the state of a resource when a render pass begins, and the state of that resource when the
     * render pass ends
     */
    using BeginEndState = Rx::Pair<D3D12_RESOURCE_STATES, D3D12_RESOURCE_STATES>;

    /*!
     * \brief Simple abstraction for a render pass
     */
    class RenderPass {
    public:
        virtual ~RenderPass() = default;

        virtual void render(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx, const World& world) = 0;

        [[nodiscard]] const Rx::Map<TextureHandle, BeginEndState>& get_texture_states() const;

    protected:
        /*!
         * \brief Describes how this renderpass will use a resource
         *
         * By default, a resource will end the renderpass with the same states it had when the renderpass began.
         * However, this is not always desired behavior. This method allows you to specify a different begin and end
         * state, so that the renderpass itself may transition resource state
         *
         * \param handle A handle to the texture to mark the usage of
         * \param begin_states The states that the resource must be in when this render pass begins
         * \param end_states The states that this resource will be in when this render pass ende. The default value of
         * D3D12_RESOURCE_STATE_COMMON means "at the end of this renderpass, the resource will have the same states it
         * had at the beginning". I feel gross using a valid resource state as my special marker, but also I don't see
         * a good way to get an invalid resource state? Any magic number I choose might get used by D3D12 itself in
         * the next update. COMMON, however, is special even to D3D12 - because resources get promoted to the common 
         * state after a command list executes. I don't expect to ever intentionally use it
         */
        void add_resource_usage(TextureHandle handle,
                                D3D12_RESOURCE_STATES begin_states,
                                D3D12_RESOURCE_STATES end_states = D3D12_RESOURCE_STATE_COMMON);

    private:
        Rx::Map<TextureHandle, Rx::Pair<D3D12_RESOURCE_STATES, D3D12_RESOURCE_STATES>> texture_states;
    };
} // namespace renderer
