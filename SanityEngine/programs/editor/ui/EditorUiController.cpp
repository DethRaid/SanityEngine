#include "EditorUiController.hpp"

#include <entt/entity/registry.hpp>

#include "ApplicationGui.hpp"
#include "adapters/rex/rex_wrapper.hpp"
#include "ui/ui_components.hpp"

namespace sanity::editor::ui {
    EditorUiController::EditorUiController(entt::registry& registry) { create_application_menu(registry); }

    void EditorUiController::create_application_menu(entt::registry& registry) {
        application_gui = registry.create();

        registry.emplace<::ui::UiComponent>(application_gui, Rx::make_ptr<ApplicationGui>(RX_SYSTEM_ALLOCATOR, *this));
    }
} // namespace sanity::editor::ui
