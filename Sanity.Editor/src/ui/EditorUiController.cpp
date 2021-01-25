#include "EditorUiController.hpp"

#include "actor/actor.hpp"
#include "entity/entity_operations.hpp"
#include "sanity_engine.hpp"
#include "ui/Window.hpp"
#include "windows/EntityEditorWindow.hpp"
#include "windows/WorldgenParamsEditor.hpp"
#include "windows/content_browser.hpp"
#include "windows/mesh_import_window.hpp"
#include "windows/scene_hierarchy.hpp"
#include "world/world.hpp"

namespace sanity::editor::ui {
    EditorUiController::EditorUiController() {
        auto& registry = engine::g_engine->get_entity_registry();

        worldgen_params_editor = create_window_entity<WorldgenParamsEditor>(registry);
        worldgen_params_editor->is_visible = false;

        content_browser = create_window_entity<ContentBrowser>(registry);

        scene_hierarchy = create_window_entity<SceneHierarchy>(registry, registry, *this);
        scene_hierarchy->is_visible = true;
    }

    void EditorUiController::show_worldgen_params_editor() const { worldgen_params_editor->is_visible = true; }

    EntityEditorWindow* EditorUiController::show_edit_entity_window(const entt::entity entity, entt::registry& registry) const {
        auto* entity_editor_window = create_window_entity<EntityEditorWindow>(registry, entity, registry);

        entity_editor_window->is_visible = true;

        return entity_editor_window;
    }

    void EditorUiController::create_and_edit_new_entity() const {
        auto& registry = engine::g_engine->get_entity_registry();

        const auto& new_entity = engine::create_actor(registry, "New Entity");

        show_edit_entity_window(new_entity.entity, registry);
    }

    void EditorUiController::set_content_browser_directory(const std::filesystem::path& content_directory) const {
        if(content_browser != nullptr) {
            content_browser->set_content_directory(content_directory);
        }
    }

    void EditorUiController::show_scene_hierarchy_window() const {
        if(scene_hierarchy != nullptr) {
            scene_hierarchy->is_visible = true;
        }
    }

    void EditorUiController::show_editor_for_asset(const std::filesystem::path& asset_path) const {
        const auto extension = asset_path.extension();
        if(extension == ".glb" || extension == ".gltf") {
            open_mesh_import_settings(asset_path);
        }
    }

    void EditorUiController::open_mesh_import_settings(const std::filesystem::path& mesh_path) const {
        auto& registry = engine::g_engine->get_entity_registry();

        auto* window = create_window_entity<SceneImportWindow>(registry, mesh_path);
        window->is_visible = true;
    }
} // namespace sanity::editor::ui
