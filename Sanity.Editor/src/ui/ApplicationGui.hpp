#pragma once

#include "ui/Window.hpp"

namespace sanity::editor::ui {
    class EditorUiController;

    class ApplicationGui final : public engine::ui::UiPanel {
    public:
        explicit ApplicationGui(EditorUiController& ui_controller_in);

        ~ApplicationGui() override = default;

        void draw() override;

    private:
        EditorUiController* ui_controller{nullptr};

#pragma region Application menu bar    	
        void draw_application_menu() const;

        void draw_window_menu() const;
    	
        void draw_world_menu() const;

        void draw_entity_menu() const;
#pragma endregion
    };
} // namespace sanity::editor::ui
