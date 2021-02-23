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
        enum class State {
            Default,
            AddingComponent,
        };

        entt::entity entity;

        entt::registry* registry{nullptr};

        Uint32 cur_component_type_idx{0};

        GUID selected_component_type_guid{};

        State state{State::Default};

        void draw_component_type_list(engine::Actor& actor);
    };
} // namespace sanity::editor::ui
