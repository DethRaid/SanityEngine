#pragma once

#include "asset_registry/asset_registry.hpp"
#include "import/scene_importer.hpp"
#include "ui/Window.hpp"

namespace sanity::editor::ui {
    class SceneImportWindow : public engine::ui::Window {
    public:
        explicit SceneImportWindow(const Rx::String& mesh_path_in);

    protected:
        void draw_contents() override;

    private:
        Rx::Ptr<import::SceneImporter> importer;

        AssetMetadata<SceneImportSettings> metadata{};

        Rx::String mesh_path;
    	
        void import_scene() const;
    };
} // namespace sanity::editor::ui
