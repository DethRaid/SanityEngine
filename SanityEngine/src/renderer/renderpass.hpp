#pragma once

#include <d3d12.h>
#include <entt/entity/fwd.hpp>
#include <rx/core/types.h>

namespace renderer {
    /*!
     * \brief Simple abstraction for a render pass
     */
    class RenderPass {
    public:
        virtual ~RenderPass() = default;

        virtual void execute(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) = 0;
    };
} // namespace renderer
