#pragma once

#include "asset_registry/asset_registry.hpp"
#include "player/flycam_controller.hpp"
#include "ui/EditorUiController.hpp"

namespace sanity::editor {
    class SanityEditor {
    public:
        explicit SanityEditor(const char* executable_directory);

        void run_until_quit();

        [[nodiscard]] engine::AssetLoader& get_asset_loader() const;

        [[nodiscard]] ui::EditorUiController& get_ui_controller();

        [[nodiscard]] AssetRegistry& get_asset_registry();

    private:
        ui::EditorUiController ui_controller;

        FlycamController flycam;

        AssetRegistry asset_registry;

        Rx::Ptr<engine::AssetLoader> asset_loader;

        void create_application_gui();
    };

    inline SanityEditor* g_editor{nullptr};

    void initialize_editor(const char* executable_directory);
} // namespace sanity::editor
