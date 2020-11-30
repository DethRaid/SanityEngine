#include "EditorUiController.hpp"

namespace sanity::editor::ui {
    EditorUiController::EditorUiController() {}

    void EditorUiController::draw() {
        if(worldgen_params_editor.is_visible) {
            worldgen_params_editor.draw();
        }
    }

    void EditorUiController::show_worldgen_params_editor() { worldgen_params_editor.is_visible = true; }
} // namespace sanity::editor::ui
