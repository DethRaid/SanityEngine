#pragma once

#include "entt/forward.hpp"
#include "renderer/renderpass.hpp"

namespace renderer {
    /*!
     * \brief Fallback main pass that calculates lighting with analytical techniques
     *
     * - Render all the opaque objects with forward lighting
     */
    class ForwardLightingPass final : public RenderPass {
    public:
        explicit ForwardLightingPass() = default;

        ~ForwardLightingPass() override = default;

        // Inherited via RenderPass
        virtual void render(ID3D12GraphicsCommandList* commands, entt::registry& registry, Uint32 frame_idx, const World& world) override;
    };
} // namespace renderer
