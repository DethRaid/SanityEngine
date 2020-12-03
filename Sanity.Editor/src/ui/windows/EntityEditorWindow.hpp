#pragma once

#include "entt/entity/entity.hpp"
#include "entt/entity/registry.hpp"
#include "ui/Window.hpp"

namespace sanity::editor::ui {
    class EntityEditorWindow : public engine::ui::Window {
    public:
        explicit EntityEditorWindow(entt::entity& entity_in, entt::registry& registry_in);

    protected:
        void draw_contents() override;

    private:
        entt::entity& entity{};
        entt::registry& registry{};
    };
} // namespace sanity::editor::ui
