#include "ApplicationGui.hpp"

#include "EditorUiController.hpp"
#include "imgui/imgui.h"

namespace sanity::editor::ui {
    ApplicationGui::ApplicationGui(EditorUiController& ui_controller_in)
        : ui_controller{&ui_controller_in} {}

    void ApplicationGui::draw()
    {
	    draw_application_menu();
    }

    void ApplicationGui::draw_application_menu() {
        ImGui::BeginMainMenuBar();
        if(ImGui::BeginMenu("World")) {
            draw_world_menu();

            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Edit")) {
            draw_edit_menu();

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    void ApplicationGui::draw_world_menu() const {
        if(ImGui::MenuItem("Edit worldgen params")) {
            if(ui_controller != nullptr) {
                ui_controller->show_worldgen_params_editor();
            }
        }
    }

    void ApplicationGui::draw_edit_menu() {}
} // namespace sanity::editor::ui
