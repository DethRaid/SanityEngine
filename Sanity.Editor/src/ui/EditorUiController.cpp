#include "EditorUiController.hpp"

#include "entity/Components.hpp"
#include "rx/core/ptr.h"
#include "rx/core/utility/move.h"
#include "sanity_engine.hpp"
#include "ui/Window.hpp"
#include "windows/EntityEditorWindow.hpp"
#include "windows/WorldgenParamsEditor.hpp"
#include "windows/content_browser.hpp"

namespace sanity::editor::ui {
    EditorUiController::EditorUiController() {
        auto registry = engine::g_engine->get_global_registry().lock();

        create_worldgen_params_editor(registry);

        create_content_browser(*registry);
    }

    void EditorUiController::show_worldgen_params_editor() const { worldgen_params_editor->is_visible = true; }

    EntityEditorWindow* EditorUiController::show_edit_entity_window(entt::entity& entity, entt::registry& registry) const {
        const auto window_entity = registry.create();

        auto window_ptr = Rx::make_ptr<EntityEditorWindow>(RX_SYSTEM_ALLOCATOR, entity, registry);
        const auto& comp = registry.emplace<engine::ui::UiComponent>(window_entity, Rx::Utility::move(window_ptr));
        auto* entity_editor_window = static_cast<EntityEditorWindow*>(comp.panel.get());
        entity_editor_window->is_visible = true;

        return entity_editor_window;
    }

    void EditorUiController::create_and_edit_new_entity() const {
        auto registry = engine::g_engine->get_global_registry().lock();

        auto new_entity = registry->create();

        auto& entity_component = registry->emplace<engine::SanityEngineEntity>(new_entity);
        entity_component.name = "New Entity";
        registry->emplace<engine::TransformComponent>(new_entity);

        auto& component_list = registry->emplace<ComponentClassIdList>(new_entity);
        component_list.class_ids.push_back(_uuidof(engine::SanityEngineEntity));
        component_list.class_ids.push_back(_uuidof(engine::TransformComponent));

        show_edit_entity_window(new_entity, *registry);
    }

    void EditorUiController::show_content_browser() const {
        if(content_browser != nullptr) {
            content_browser->is_visible = true;
        }
    }

    void EditorUiController::create_worldgen_params_editor(SynchronizedResourceAccessor<entt::registry, Rx::Concurrency::Mutex>& registry) {
        const auto entity = registry->create();
        const auto& comp = registry->emplace<engine::ui::UiComponent>(entity, Rx::make_ptr<WorldgenParamsEditor>(RX_SYSTEM_ALLOCATOR));
        worldgen_params_editor = static_cast<WorldgenParamsEditor*>(comp.panel.get());
        worldgen_params_editor->is_visible = false;
    }

    void EditorUiController::create_content_browser(entt::registry& registry) {
        const auto entity = registry.create();

    	// Totally hardcoded content directory. In theory we'll have a way to select a project to open, in practice I'm
    	// not sure I care
    	const auto& content_directory = Rx::String::format("%s/content", engine::SanityEngine::executable_directory);
        auto content_browser_ptr = Rx::make_ptr<ContentBrowser>(RX_SYSTEM_ALLOCATOR, content_directory);
    	const auto& comp = registry.emplace<engine::ui::UiComponent>(entity, Rx::Utility::move(content_browser_ptr));

    	content_browser = static_cast<ContentBrowser*>(comp.panel.get());
        content_browser->is_visible = true;
    }
} // namespace sanity::editor::ui
