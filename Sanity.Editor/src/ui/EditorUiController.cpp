#include "EditorUiController.hpp"

#include "rx/core/ptr.h"
#include "rx/core/utility/move.h"
#include "sanity_engine.hpp"
#include "ui/Window.hpp"
#include "windows/EntityEditorWindow.hpp"
#include "windows/WorldgenParamsEditor.hpp"

namespace sanity::editor::ui {
    EditorUiController::EditorUiController() {
        auto registry = engine::g_engine->get_global_registry().lock();

        create_worldgen_params_editor(registry);
    }

    void EditorUiController::show_worldgen_params_editor() const { worldgen_params_editor->is_visible = true; }

    EntityEditorWindow* EditorUiController::show_edit_entity_window(entt::entity& entity, entt::registry& registry) const {
        const auto window_entity = registry.create();

        auto window_ptr = Rx::make_ptr<EntityEditorWindow>(RX_SYSTEM_ALLOCATOR, entity, registry);
        const auto& comp = registry.emplace<engine::ui::UiComponent>(window_entity, Rx::Utility::move(window_ptr));
        auto* entity_editor_window = static_cast<EntityEditorWindow*>(comp.panel.get());

        return entity_editor_window;
    }

    void EditorUiController::create_worldgen_params_editor(SynchronizedResourceAccessor<entt::registry, Rx::Concurrency::Mutex>& registry) {
        const auto entity = registry->create();
        const auto& comp = registry->emplace<engine::ui::UiComponent>(entity, Rx::make_ptr<WorldgenParamsEditor>(RX_SYSTEM_ALLOCATOR));
        worldgen_params_editor = static_cast<WorldgenParamsEditor*>(comp.panel.get());
        worldgen_params_editor->is_visible = false;
    }
} // namespace sanity::editor::ui
