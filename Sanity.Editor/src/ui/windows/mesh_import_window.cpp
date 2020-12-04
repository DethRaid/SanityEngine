#include "mesh_import_window.hpp"

#include "ui/property_drawers.hpp"

using namespace sanity::engine::ui;

namespace sanity::editor::ui {
    MeshImportWindow::MeshImportWindow(const Rx::String& mesh_path)
        : metadata{AssetRegistry::get_meta_for_asset<MeshImportSettings>(mesh_path)} {
        const auto slash_pos = mesh_path.find_last_of("/");
        const auto filename = mesh_path.substring(slash_pos + 1);
        name = Rx::String::format("Import %s", filename);
    }

    void MeshImportWindow::draw_contents() {
        auto& import_settings = metadata.import_settings;

        draw_property_editor("Import materials", import_settings.import_materials);

        if(ImGui::Button("Import")) {
            // TODO
        }
    }
} // namespace sanity::editor::ui
