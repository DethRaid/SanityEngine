#pragma once

#include "renderer/renderpass.hpp"
#include "renderer/rhi/render_pipeline_state.hpp"
#include "rx/core/ptr.h"

namespace renderer {
    class Renderer;

	/*!
	 * \brief Renders all the UI that's been drawn with ImGUI since the last frame
	 */
    class DearImGuiRenderPass final : public RenderPass {
    public:
        explicit DearImGuiRenderPass(Renderer& renderer_in);

        ~DearImGuiRenderPass() override = default;

        void render(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx, const World& world) override;

    private:
        Renderer* renderer;

        Rx::Ptr<RenderPipelineState> ui_pipeline;
    };
} // namespace renderer
