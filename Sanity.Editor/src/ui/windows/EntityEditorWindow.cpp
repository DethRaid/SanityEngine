#include "EntityEditorWindow.hpp"

#include "actor/actor.hpp"
#include "core/components.hpp"
#include "entity/Components.hpp"
#include "renderer/render_components.hpp"
#include "sanity_engine.hpp"

using namespace sanity::engine::ui;

namespace sanity::editor::ui {
    void draw_component(const GUID& component_type_id, const entt::entity& entity, entt::registry& registry);

    EntityEditorWindow::EntityEditorWindow(const entt::entity entity_in, entt::registry& registry_in)
        : Window{"Entity Editor"}, entity{entity_in}, registry{&registry_in} {
        const auto& sanity_component = registry->get<engine::Actor>(entity);
        if(!sanity_component.name.is_empty()) {
            name = sanity_component.name;
        }
    }

    void EntityEditorWindow::set_entity(const entt::entity new_entity, entt::registry& new_registry) {
        if(entity != new_entity || registry != &new_registry) {
            entity = new_entity;
            registry = &new_registry;

            const auto& sanity_component = registry->get<engine::Actor>(entity);
            if(!sanity_component.name.is_empty()) {
                name = sanity_component.name;
            }
        }
    }

    void EntityEditorWindow::draw_contents() {
        const auto sanity_entity = registry->get<engine::Actor>(entity);

        ImGui::Text("%s", sanity_entity.name.data());

        if(ImGui::Button("Add Component")) {
            // Open add component.... window? selection?
        }

        sanity_entity.component_class_ids.each_fwd([&](const GUID class_id) {
            ImGui::Separator();
            draw_component(class_id, entity, *registry);
        });
    }

#define DRAW_COMPONENT_EDITOR(Type)                                                                                                        \
    if(component_type_id == _uuidof(Type)) {                                                                                               \
        auto& component = registry.get<Type>(entity);                                                                                      \
        if(ImGui::CollapsingHeader(class_name.data())) {                                                                                   \
            ImGui::Indent();                                                                                                               \
            draw_component_properties(component);                                                                                          \
            ImGui::Unindent();                                                                                                             \
        }                                                                                                                                  \
    }

    void draw_component(const GUID& component_type_id, const entt::entity& entity, entt::registry& registry) {
        const auto class_name = engine::g_engine->get_type_reflector().get_name_of_type(component_type_id);

        DRAW_COMPONENT_EDITOR(engine::Actor)
        DRAW_COMPONENT_EDITOR(engine::TransformComponent)
        DRAW_COMPONENT_EDITOR(engine::renderer::StandardRenderableComponent)
        DRAW_COMPONENT_EDITOR(engine::renderer::PostProcessingPassComponent)
        DRAW_COMPONENT_EDITOR(engine::renderer::RaytracingObjectComponent)
        DRAW_COMPONENT_EDITOR(engine::renderer::CameraComponent)
        DRAW_COMPONENT_EDITOR(engine::renderer::LightComponent)
        DRAW_COMPONENT_EDITOR(engine::renderer::SkyComponent);
    }
} // namespace sanity::editor::ui
