#pragma once

#include <d3d12.h>

#include "core/types.hpp"
#include "entt/entity/fwd.hpp"
#include "renderer/handles.hpp"
#include "rhi/resources.hpp"
#include "rx/core/map.h"
#include "rx/core/optional.h"

namespace sanity::engine {
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

            virtual void render(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) = 0;

            [[nodiscard]] const Rx::Map<TextureHandle, Rx::Optional<BeginEndState>>& get_texture_states() const;

        protected:
            /*!
             * \brief Describes how this renderpass will use a resource
             *
             * \param handle A handle to the texture to mark the usage of
             * \param states The states of this resource during this renderpass
             */
            void add_resource_usage(TextureHandle handle, D3D12_RESOURCE_STATES states);

            /*!
             * \brief Describes how this renderpass will use a resource
             *
             * This method allows you to set a different begin and end state of a resource. You are expected to get the
             * resource from its begin state to its end state within your override of `render`
             *
             * \param handle A handle to the texture to mark the usage of
             * \param begin_states The states that the resource must be in when this render pass begins
             * \param end_states The states that this resource will be in when this render pass ends
             */
            void add_resource_usage(TextureHandle handle, D3D12_RESOURCE_STATES begin_states, D3D12_RESOURCE_STATES end_states);

            /**
             * @brief Removes the usage information for this resources
             *
             * @param handle A handle to the texture to remove the usage information for
             */
            void remove_resource_usage(TextureHandle handle);

        private:
            Rx::Map<TextureHandle, Rx::Optional<BeginEndState>> texture_states;
        };
    } // namespace renderer
} // namespace sanity::engine
