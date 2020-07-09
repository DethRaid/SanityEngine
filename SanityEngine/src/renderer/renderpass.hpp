#pragma once

#include <d3d12.h>

#include "core/types.hpp"
#include "entt/entity/fwd.hpp"

class World;

namespace renderer {
    /*!
     * \brief Simple abstraction for a render pass
     */
    class RenderPass {
    public:
        virtual ~RenderPass() = default;

        virtual void render(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx, const World& world) = 0;
    };
} // namespace renderer
