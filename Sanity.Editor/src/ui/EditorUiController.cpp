#include "EditorUiController.hpp"

#include "rx/core/ptr.h"
#include "sanity_engine.hpp"
#include "ui/Window.hpp"
#include "windows/WorldgenParamsEditor.hpp"

namespace sanity::editor::ui {
    EditorUiController::EditorUiController() { create_worldgen_params_editor(); }

    void EditorUiController::show_worldgen_params_editor() {
        auto registry = engine::g_engine->get_global_registry().lock();
        auto& comp = registry->get<engine::ui::UiComponent>(worldgen_params_editor);
        auto* panel = comp.panel.get();
        auto* window = static_cast<engine::ui::Window*>(panel);
        window->is_visible = true;
    }

    void EditorUiController::create_worldgen_params_editor() {
        auto registry = engine::g_engine->get_global_registry().lock();
        worldgen_params_editor = registry->create();
        const auto& comp = registry->emplace<engine::ui::UiComponent>(worldgen_params_editor,
                                                                      Rx::make_ptr<WorldgenParamsEditor>(RX_SYSTEM_ALLOCATOR));
        auto* window = static_cast<engine::ui::Window*>(comp.panel.get());
        window->is_visible = false;
    }
} // namespace sanity::editor::ui
