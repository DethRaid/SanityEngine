#include "world_preview_panel.hpp"

#include "renderer/rhi/render_backend.hpp"

namespace sanity::engine::ui {
    WorldPreviewPane::WorldPreviewPane(const renderer::Renderer& renderer_in) : UiPanel{"World Preview Pane"}, renderer{&renderer_in} {
        // TODO: Get the texture index for Scene output render texture
    }

    void WorldPreviewPane::draw() {
        if(ImGui::Begin(name.data())) {
            ImGui::Image(renderer_output_texture, ImVec2{});
        }
        ImGui::End();
    }
} // namespace sanity::engine::ui
