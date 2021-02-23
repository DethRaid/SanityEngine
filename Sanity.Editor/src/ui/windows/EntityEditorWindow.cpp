#include "EntityEditorWindow.hpp"

#include <ui/property_drawers.hpp>

#include "SanityEditor.hpp"
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
        auto& actor = registry->get<engine::Actor>(entity);

        ImGui::Text("%s", actor.name.data());

        switch(state) {
            case State::Default:
                if(ImGui::Button("Add Component")) {
                    state = State::AddingComponent;
                }
                break;

            case State::AddingComponent: {
                draw_component_type_list(actor);

                if(ImGui::Button("Add")) {
                    actor.add_component(selected_component_type_guid);
                    state = State::Default;
                }
                ImGui::SameLine();
                if(ImGui::Button("Cancel")) {
                    state = State::Default;
                }
            } break;
        }

        actor.component_class_ids.each_fwd([&](const GUID class_id) {
            ImGui::Separator();
            draw_component(class_id, entity, *registry);
        });
    }

#define MAYBE_ADD_COMPONENT(Type)                                                                                                          \
    if(component_type_id == _uuidof(Type)) {                                                                                               \
        actor.add_component<Type>();                                                                                                       \
    }

    void EntityEditorWindow::draw_component_type_list(engine::Actor& actor) {
        const auto& type_reflection = engine::g_engine->get_type_reflector();

        // At the time of writing, the type reflector only has information about components. No filtering is necessary
        const auto& type_names = type_reflection.get_type_names();

        // Names of components that are not already added to this actor
        auto available_component_names = Rx::Vector<Rx::String>();
        auto guids = Rx::Vector<GUID>();

        // TODO: Sort the component names intelligently
        type_names.each_pair([&](const GUID& guid, const Rx::String& component_type_name) {
            if(actor.has_component(guid)) {
                return RX_ITERATION_CONTINUE;
            }

            available_component_names.push_back(component_type_name.data());
            guids.push_back(guid);

            return RX_ITERATION_CONTINUE;
        });

        ImGui::Separator();

        draw_drop_down_selector("Component Type", available_component_names, cur_component_type_idx);
        selected_component_type_guid = guids[cur_component_type_idx];
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
        DRAW_COMPONENT_EDITOR(engine::renderer::SkyComponent)
        DRAW_COMPONENT_EDITOR(engine::renderer::FluidVolumeComponent)
    }
} // namespace sanity::editor::ui
