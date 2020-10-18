#pragma once
#include "ui/ui_panel.hpp"

namespace sanity::editor::ui {
    class EditorUiController;

    class ApplicationGui final : public ::ui::UiPanel {
    public:
        explicit ApplicationGui(EditorUiController& controller_in);

        ~ApplicationGui() override = default;

        void draw() override;

    private:
        EditorUiController* ui_controller{nullptr};

#pragma region Application menu bar
        void draw_application_menu();

        void draw_world_menu() const;

        void draw_edit_menu();
#pragma endregion
    };
} // namespace sanity::editor::ui
