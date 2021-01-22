#include "scene_hierarchy.hpp"

#include <core/components.hpp>

#include "entity/Components.hpp"
#include "ui/EditorUiController.hpp"

namespace sanity::editor::ui {
    SceneHierarchy::SceneHierarchy(entt::registry& registry_in, EditorUiController& controller_in)
        : Window{"Scene Hierarchy"}, registry(&registry_in), controller{&controller_in} {}

    void SceneHierarchy::draw_contents() {
        auto entities = registry->view<engine::SanityEngineEntity>();

        // Collect entities in the root of the scene
        std::vector<entt::entity> root_entities{};
        root_entities.reserve(entities.size());

        for(auto entity : entities) {
            const auto& transform = registry->get<engine::TransformComponent>(entity);
            if(!transform.parent) {
                root_entities.push_back(entity);
            }
        }

        // Render a list of the entities just to be sure this whole thing works
        for(const auto entity : root_entities) {
            draw_entity(entity);
        }
    }

    void SceneHierarchy::draw_entity(const entt::entity entity) {
        const auto& sanity_entity = registry->get<engine::SanityEngineEntity>(entity);
        const auto& transform = registry->get<engine::TransformComponent>(entity);

        ImGui::PushID(sanity_entity.name.data());
    	
        ImGui::Text("%s", sanity_entity.name.data());
        ImGui::SameLine();
        if(ImGui::Button("Inspect")) {
            show_entity_editor(entity);
        }
    	
        if(!transform.children.is_empty()) {
            if(ImGui::CollapsingHeader("Children")) {
                ImGui::Indent();
                transform.children.each_fwd([&](const entt::entity& child) { draw_entity(child); });
                ImGui::Unindent();
            } 
        }

    	ImGui::PopID();
    }

    void SceneHierarchy::show_entity_editor(const entt::entity entity) {
        if(entity_editor == nullptr) {
            entity_editor = controller->show_edit_entity_window(entity, *registry);

        } else {
            entity_editor->set_entity(entity, *registry);
        }

    	entity_editor->is_visible = true;
    }
} // namespace sanity::editor::ui
