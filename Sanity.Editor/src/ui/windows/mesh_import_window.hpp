#pragma once

#include "asset_registry/asset_registry.hpp"
#include "ui/Window.hpp"

namespace sanity::editor::ui {
    class SceneImportWindow : public engine::ui::Window {
    public:
        explicit SceneImportWindow(const Rx::String& mesh_path_in);

    protected:
        void draw_contents() override;

    private:
        AssetMetadata<SceneImportSettings> metadata{};

        Rx::String mesh_path;
    };
} // namespace sanity::editor::ui
