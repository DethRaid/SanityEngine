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

            /**
             * @brief Examines the state of the world and makes note of any GPU operations that are needed
             *
             * Think of it as recording a high-level command list of GPU work. Except most of Sanity's renderer doesn't use the two-pass
             * system and instead does everything in `record_work`. It's on the TODO list, I promise.......
             *
             * @note You can and should make calls to `set_resource_usage` in this method
             */
            virtual void collect_work(entt::registry& registry, Uint32 frame_idx);

            /**
             * @brief Records this pass's work into a GPU command list
             * 
             * @param commands Command list to record work to
             * @param registry EnTT registry for the world. Hopefully eventually only collect_work will need this
             * @param frame_idx Index of the GPU frame to record work for
            */
            virtual void record_work(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) = 0;

            [[nodiscard]] const Rx::Map<TextureHandle, Rx::Optional<BeginEndState>>& get_texture_states() const;

        protected:
            /*!
             * \brief Describes how this renderpass will use a resource
             *
             * \param handle A handle to the texture to mark the usage of
             * \param states The states of this resource during this renderpass
             */
            void set_resource_usage(TextureHandle handle, D3D12_RESOURCE_STATES states);

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
            void set_resource_usage(TextureHandle handle, D3D12_RESOURCE_STATES begin_states, D3D12_RESOURCE_STATES end_states);

            /**
             * @brief Removes the usage information for this resources
             *
             * @param handle A handle to the texture to remove the usage information for
             */
            void clear_resource_usage(TextureHandle handle);

        private:
            Rx::Map<TextureHandle, Rx::Optional<BeginEndState>> texture_states;
        };
    } // namespace renderer
} // namespace sanity::engine
