#include "ApplicationGui.hpp"

#include "EditorUiController.hpp"
#include "SanityEditor.hpp"
#include "imgui/imgui.h"

namespace sanity::editor::ui {
    ApplicationGui::ApplicationGui(EditorUiController& ui_controller_in) : UiPanel{"Editor UI"}, ui_controller{&ui_controller_in} {}

    void ApplicationGui::draw() { draw_application_menu(); }

    void ApplicationGui::draw_application_menu() const {
        ImGui::BeginMainMenuBar();

        if(ImGui::BeginMenu("Window")) {
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

    void ApplicationGui::draw_window_menu() const {
        if(ImGui::MenuItem("Content Browser")) {
            const auto& content_dir = g_editor->get_content_directory();
            ui_controller->set_content_browser_directory(content_dir);
        }
    }

    void ApplicationGui::draw_world_menu() const {
        if(ImGui::MenuItem("Edit worldgen params")) {
            ui_controller->show_worldgen_params_editor();
        }
    }

    void ApplicationGui::draw_entity_menu() const {
        if(ImGui::MenuItem("New entity")) {
            ui_controller->create_and_edit_new_entity();
        }
    }
} // namespace sanity::editor::ui
