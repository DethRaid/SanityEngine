#pragma once

#include "player/flycam_controller.hpp"

#include "ui/EditorUiController.hpp"

namespace sanity::editor {
    class SanityEditor {
    public:
        explicit SanityEditor(const char* executable_directory);

        void run_until_quit();

    private:
        ui::EditorUiController ui_controller;
    	
        FlycamController flycam;

        void create_application_gui();
    };
} // namespace sanity::editor
