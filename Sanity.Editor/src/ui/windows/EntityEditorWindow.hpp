#pragma once

#include <actor/actor.hpp>

#include "entt/entity/entity.hpp"
#include "entt/entity/registry.hpp"
#include "glm/vec2.hpp"
#include "ui/Window.hpp"

namespace sanity::editor::ui {
    class EntityEditorWindow : public engine::ui::Window {
    public:
        explicit EntityEditorWindow(entt::entity entity_in, entt::registry& registry_in);

        void set_entity(entt::entity new_entity, entt::registry& new_registry);

    protected:
        void draw_contents() override;

    private:
        entt::entity entity;

        entt::registry* registry{nullptr};

        bool should_show_component_type_list{false};
        glm::uvec2 component_type_list_location{};

        void draw_component_type_list(engine::Actor& actor);
    };
} // namespace sanity::editor::ui
