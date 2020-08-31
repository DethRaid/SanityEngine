#include "world_preview_panel.hpp"

#include "rhi/render_device.hpp"

namespace ui {
    WorldPreviewPane::WorldPreviewPane(const renderer::Renderer& renderer_in) : renderer{&renderer_in} {}

    void WorldPreviewPane::draw() {
        if(ImGui::Begin("World Preview")) {
            ImGui::Image(renderer_output_texture, ImVec2{});
        }
        ImGui::End();
    }
} // namespace ui
