#include "world_preview_panel.hpp"

#include "renderer/rhi/render_device.hpp"

namespace sanity::engine::ui {
    WorldPreviewPane::WorldPreviewPane(const renderer::Renderer& renderer_in) : renderer{&renderer_in} {
        // TODO: Get the texture index for Scene output render texture
    }

    void WorldPreviewPane::draw() {
        if(ImGui::Begin("World Preview")) {
            ImGui::Image(renderer_output_texture, ImVec2{});
        }
        ImGui::End();
    }
} // namespace ui
