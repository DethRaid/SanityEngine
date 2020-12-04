#include "EntityEditorWindow.hpp"

#include "core/class_id.hpp"
#include "core/components.hpp"
#include "entity/Components.hpp"
#include "ui/property_drawers.hpp"

using namespace sanity::engine::ui;

namespace sanity::editor::ui {
    void draw_component_editor(const GUID& class_id, const entt::entity& entity, entt::registry& registry);

    EntityEditorWindow::EntityEditorWindow(entt::entity& entity_in, entt::registry& registry_in)
        : Window{"Entity Editor"}, entity{entity_in}, registry{registry_in} {
        const auto& sanity_component = registry.get<engine::SanityEngineEntity>(entity);
        if(!sanity_component.name.is_empty()) {
            name = sanity_component.name;
        }
    }

    void EntityEditorWindow::draw_contents() {
        const auto component_list = registry.get<ComponentClassIdList>(entity);
        component_list.class_ids.each_fwd([&](const GUID class_id) { draw_component_editor(class_id, entity, registry); });
    }

    void draw_component_editor(engine::TransformComponent& transform);

    void draw_component_editor(const GUID& class_id, const entt::entity& entity, entt::registry& registry) {
        const auto class_name = engine::class_name_from_guid(class_id);
        if(ImGui::CollapsingHeader(class_name.data())) {
            if(class_id == _uuidof(engine::TransformComponent)) {
                auto& component = registry.get<engine::TransformComponent>(entity);
                draw_component_editor(component);
            }
        }
    }

    void draw_component_editor(engine::TransformComponent& transform) {
        vec3_property("location", transform.location);
        quat_property("rotation", transform.rotation);
        vec3_property("scale", transform.scale);
    }

} // namespace sanity::editor::ui
