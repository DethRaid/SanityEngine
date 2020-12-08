#pragma once

#include "asset_registry/asset_registry.hpp"
#include "player/flycam_controller.hpp"
#include "project/project_definition.hpp"
#include "ui/EditorUiController.hpp"

namespace sanity::editor {
    class SanityEditor {
    public:
        explicit SanityEditor(const Rx::String& initial_project_file);

        void run_until_quit();

        [[nodiscard]] engine::AssetLoader& get_asset_loader() const;

        [[nodiscard]] ui::EditorUiController& get_ui_controller();

        [[nodiscard]] AssetRegistry& get_asset_registry();

        [[nodiscard]] const Rx::String& get_content_directory() const;

    private:
        ui::EditorUiController ui_controller;

        FlycamController flycam;

        AssetRegistry asset_registry;

        Rx::Ptr<engine::AssetLoader> asset_loader;

        /*!
         * \brief Content directory for the currently selected project
         */
        Rx::String content_directory;

        Project project_data;

        void load_project(const Rx::String& project_file);

        void create_application_gui();
    };

    inline SanityEditor* g_editor{nullptr};

    SanityEditor* initialize_editor(const Rx::String& initial_project_directory);
} // namespace sanity::editor
