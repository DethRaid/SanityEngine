#pragma once

#include <d3d12.h>
#include <entt/entity/fwd.hpp>

namespace renderer {
    /*!
     * \brief Simple abstraction for a render pass
     */
    class RenderPass {
    public:
        virtual ~RenderPass() = default;

        virtual void execute(ID3D12GraphicsCommandList4* commands, entt::registry& registry, uint32_t frame_idx) = 0;
    };
}
