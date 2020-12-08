#include "EditorUiController.hpp"

#include "entity/Components.hpp"
#include "entity/entity_operations.hpp"
#include "rx/core/filesystem/file.h"
#include "sanity_engine.hpp"
#include "ui/Window.hpp"
#include "windows/EntityEditorWindow.hpp"
#include "windows/WorldgenParamsEditor.hpp"
#include "windows/content_browser.hpp"
#include "windows/mesh_import_window.hpp"

namespace sanity::editor::ui {
    EditorUiController::EditorUiController() {
        auto registry = engine::g_engine->get_global_registry().lock();

        create_worldgen_params_editor(registry);

        create_content_browser(*registry);
    }

    void EditorUiController::show_worldgen_params_editor() const { worldgen_params_editor->is_visible = true; }

    EntityEditorWindow* EditorUiController::show_edit_entity_window(entt::entity& entity, entt::registry& registry) const {
        auto* entity_editor_window = create_window_entity<EntityEditorWindow>(registry, entity, registry);

        entity_editor_window->is_visible = true;

        return entity_editor_window;
    }

    void EditorUiController::create_and_edit_new_entity() const {
        auto registry = engine::g_engine->get_global_registry().lock();

        auto new_entity = entity::create_base_editor_entity("New Entity", *registry);

        show_edit_entity_window(new_entity, *registry);
    }

    void EditorUiController::set_content_browser_directory(const Rx::String& content_directory) const {
        if(content_browser != nullptr) {
            content_browser->set_content_directory(content_directory);
        }
    }

    void EditorUiController::show_editor_for_asset(const Rx::String& asset_path) const {
        const auto dot_pos = asset_path.find_last_of(".");
        const auto extension = asset_path.substring(dot_pos + 1);
        if(extension == "glb" || extension == "gltf") {
            open_mesh_import_settings(asset_path);
        }
    }

    void EditorUiController::open_mesh_import_settings(const Rx::String& mesh_path) const {
        auto registry = engine::g_engine->get_global_registry().lock();

    	auto* window = create_window_entity<SceneImportWindow>(*registry, mesh_path);
        window->is_visible = true;
    }

    void EditorUiController::create_worldgen_params_editor(SynchronizedResourceAccessor<entt::registry, Rx::Concurrency::Mutex>& registry) {
        worldgen_params_editor = create_window_entity<WorldgenParamsEditor>(*registry);
        worldgen_params_editor->is_visible = false;
    }

    void EditorUiController::create_content_browser(entt::registry& registry, const Rx::String& content_directory) {
        content_browser = create_window_entity<ContentBrowser>(registry, content_directory);
    	content_browser->is_visible = true;
    }
} // namespace sanity::editor::ui
