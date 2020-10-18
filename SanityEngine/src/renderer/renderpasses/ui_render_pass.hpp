#pragma once

#include "renderer/renderpass.hpp"
#include "renderer/rhi/render_pipeline_state.hpp"
#include "rx/core/ptr.h"

namespace renderer {
    class Renderer;

    class GameUiPass final : public RenderPass {
    public:
        explicit GameUiPass(Renderer& renderer_in);

        ~GameUiPass() override = default;

        void render(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx, const World& world) override;

    private:
        Renderer* renderer;

        Rx::Ptr<RenderPipelineState> ui_pipeline;
    };
} // namespace renderer
