#include "scene_viewport.hpp"

namespace sanity::editor::ui {
    SceneViewport::SceneViewport() : Window{"Scene Viewport"} {
        // Create an ImGUI texture for the Sanity texture of the renderer's scene output
    }

    void SceneViewport::draw_contents() { ImGui::Image(scene_output_texture, ImVec2{}); }
} // namespace sanity::editor::ui
