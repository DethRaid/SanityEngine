#pragma once
#include "asset_registry/asset_registry.hpp"
#include "ui/Window.hpp"

namespace sanity::editor::ui {
    class MeshImportWindow : public engine::ui::Window {
    public:
        explicit MeshImportWindow(const Rx::String& mesh_path);

    protected:
        void draw_contents() override;

    private:
        AssetMetadata<MeshImportSettings> metadata{};
    };
} // namespace sanity::editor::ui
