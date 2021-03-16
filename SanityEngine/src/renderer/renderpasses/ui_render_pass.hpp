#pragma once

#include "core/types.hpp"
#include "renderer/render_pass.hpp"
#include "renderer/rhi/render_pipeline_state.hpp"
#include "rx/core/ptr.h"

namespace sanity::engine::renderer {
    class Renderer;

    /*!
     * \brief Renders all the UI that's been drawn with ImGUI since the last frame
     */
    class DearImGuiRenderPass final : public RenderPass {
    public:
        explicit DearImGuiRenderPass(Renderer& renderer_in);

        ~DearImGuiRenderPass() override = default;

        void set_background_color(const Vec4f& color);

        void prepare_work(entt::registry& registry, Uint32 frame_idx, float delta_time) override;

        void record_work(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx, float delta_time) override;

    private:
        Renderer* renderer;

        Rx::Ptr<RenderPipelineState> ui_pipeline;

        Vec4f background_color{79.f / 255.f, 77.f / 255.f, 78.f / 255.f, 1.f};
    };
} // namespace sanity::engine::renderer
