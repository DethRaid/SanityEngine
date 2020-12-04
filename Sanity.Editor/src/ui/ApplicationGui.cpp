#include "ApplicationGui.hpp"

#include "EditorUiController.hpp"
#include "imgui/imgui.h"

namespace sanity::editor::ui {
    ApplicationGui::ApplicationGui(EditorUiController& ui_controller_in) : ui_controller{&ui_controller_in} {}

    void ApplicationGui::draw() { draw_application_menu(); }

    void ApplicationGui::draw_application_menu() {
        ImGui::BeginMainMenuBar();

    	if(ImGui::BeginMenu("Window"))
    	{
            draw_window_menu();
            ImGui::EndMenu();
    	}
    	
        if(ImGui::BeginMenu("World")) {
            draw_world_menu();

            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Entity")) {
            draw_entity_menu();

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    void ApplicationGui::draw_window_menu()
    { if(ImGui::MenuItem("Content Browser"))
    {
            ui_controller->show_content_browser();
    }
    }

    void ApplicationGui::draw_world_menu() const {
        if(ImGui::MenuItem("Edit worldgen params")) {
            ui_controller->show_worldgen_params_editor();
        }
    }

    void ApplicationGui::draw_entity_menu() {
        if(ImGui::MenuItem("New entity")) {
            ui_controller->create_and_edit_new_entity();
        }
    }
} // namespace sanity::editor::ui
