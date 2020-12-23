#pragma once

#include <filesystem>

#include "asset_registry/asset_registry.hpp"
#include "player/flycam_controller.hpp"
#include "project/project_definition.hpp"
#include "ui/EditorUiController.hpp"

namespace sanity::editor {
    class SanityEditor {
    public:
        explicit SanityEditor(const std::filesystem::path& initial_project_file);

        void run_until_quit();

        [[nodiscard]] engine::AssetLoader& get_asset_loader() const;

        [[nodiscard]] ui::EditorUiController& get_ui_controller();

        [[nodiscard]] AssetRegistry& get_asset_registry();

        [[nodiscard]] const std::filesystem::path& get_content_directory() const;

    private:
        ui::EditorUiController ui_controller;

        FlycamController flycam;

        AssetRegistry asset_registry;

        Rx::Ptr<engine::AssetLoader> asset_loader;

        /*!
         * \brief Content directory for the currently selected project
         */
        std::filesystem::path content_directory;

        Project project_data;

        void load_project(const std::filesystem::path& project_file, bool should_scan_project_directory = true);

        void scan_project_directory_async(const std::filesystem::path& project_content_directory);

        void create_application_gui();
    };

    inline SanityEditor* g_editor{nullptr};

    SanityEditor* initialize_editor(const std::filesystem::path& initial_project_directory);
} // namespace sanity::editor
