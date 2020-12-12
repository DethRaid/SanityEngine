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

    void draw_component_editor(engine::SanityEngineEntity& entity);
    void draw_component_editor(engine::TransformComponent& transform);

#define DRAW_COMPONENT_EDITOR(Type)                                                                                                        \
    if(class_id == _uuidof(Type)) {                                                                                                        \
        auto& component = registry.get<Type>(entity);                                                                                      \
        draw_component_editor(component);                                                                                                  \
    }

    void draw_component_editor(const GUID& class_id, const entt::entity& entity, entt::registry& registry) {
        const auto class_name = engine::class_name_from_guid(class_id);
        if(ImGui::CollapsingHeader(class_name.data())) {
            DRAW_COMPONENT_EDITOR(engine::SanityEngineEntity)
            else DRAW_COMPONENT_EDITOR(engine::TransformComponent)
        }
    }

    void draw_component_editor(engine::SanityEngineEntity& entity) {
        ImGui::LabelText("ID", "%ld", entity.id);
        draw_property_editor("name", entity.name);
        draw_property_editor("tags", entity.tags);
    }

    void draw_component_editor(engine::TransformComponent& transform) {
        draw_property_editor("transform", transform.transform);
    }

} // namespace sanity::editor::ui
