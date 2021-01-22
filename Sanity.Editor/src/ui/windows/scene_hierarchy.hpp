#pragma once

#include <entt/entity/registry.hpp>
#include <ui/Window.hpp>

#include "EntityEditorWindow.hpp"

namespace sanity::editor::ui {
	class EditorUiController;

	class SceneHierarchy : public engine::ui::Window
    {
    public:
        explicit SceneHierarchy(entt::registry& registry_in, EditorUiController& controller_in);

    protected:
        void draw_contents() override;

    private:
        entt::registry* registry;

		EditorUiController* controller;
		
        EntityEditorWindow* entity_editor{nullptr};

        void draw_entity(entt::entity entity);

        void show_entity_editor(entt::entity entity);
    };
} // namespace sanity::editor::ui
