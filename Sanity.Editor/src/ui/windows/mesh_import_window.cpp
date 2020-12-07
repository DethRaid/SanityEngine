#include "mesh_import_window.hpp"

#include "SanityEditor.hpp"
#include "import/scene_importer.hpp"
#include "serialization/EntitySerialization.hpp"
#include "ui/property_drawers.hpp"

using namespace sanity::engine::ui;

namespace sanity::editor::ui {
    SceneImportWindow::SceneImportWindow(const Rx::String& mesh_path_in)
        : metadata{AssetRegistry::get_meta_for_asset<SceneImportSettings>(mesh_path_in)}, mesh_path{mesh_path_in} {
        const auto slash_pos = mesh_path_in.find_last_of("/");
        const auto filename = mesh_path_in.substring(slash_pos + 1);
        name = Rx::String::format("Import %s", filename);
    }

    void SceneImportWindow::draw_contents() {
        auto& import_settings = metadata.import_settings;

    	draw_property_editor("Import meshes", import_settings.import_meshes);
        draw_property_editor("Import materials", import_settings.import_materials);
        draw_property_editor("Import lights", import_settings.import_lights);
        draw_property_editor("Import entities", import_settings.import_empties);

        if(ImGui::Button("Save")) {
            AssetRegistry::save_metadata_for_asset(mesh_path, metadata);
        }

    	ImGui::SameLine();

    	if(ImGui::Button("Import")) {
            AssetRegistry::save_metadata_for_asset(mesh_path, metadata);

    		entt::registry registry;

            const auto scene_entity = import::import_scene_into_sanity_editor(mesh_path, registry);
            if(scene_entity) {
                const auto all_jsons = serialization::entity_and_children_to_json(*scene_entity, registry);
            }
    		
    	}
    }
} // namespace sanity::editor::ui
