#include "mesh_import_window.hpp"

#include <sanity_engine.hpp>

#include "SanityEditor.hpp"
#include "core/RexJsonConversion.hpp"
#include "import/scene_importer.hpp"
#include "serialization/EntitySerialization.hpp"
#include "ui/property_drawers.hpp"

using namespace sanity::engine::ui;

namespace sanity::editor::ui {
    RX_LOG("SceneImportWindow", logger);
    SceneImportWindow::SceneImportWindow(const Rx::String& mesh_path_in)
        : metadata{AssetRegistry::get_meta_for_asset<SceneImportSettings>(mesh_path_in)}, mesh_path{mesh_path_in} {
        const auto slash_pos = mesh_path_in.find_last_of("/");
        const auto filename = mesh_path_in.substring(slash_pos + 1);
        name = Rx::String::format("Import %s", filename);

        auto& renderer = engine::g_engine->get_renderer();
        importer = Rx::make_ptr<import::SceneImporter>(RX_SYSTEM_ALLOCATOR, renderer);
    }

    void SceneImportWindow::draw_contents() {
        auto& import_settings = metadata.import_settings;

        draw_property_editor("Import meshes", import_settings.import_meshes);
        draw_property_editor("Import materials", import_settings.import_materials);
        draw_property_editor("Import lights", import_settings.import_lights);
        draw_property_editor("Import entities", import_settings.import_empties);
        draw_property_editor("Import object hierarchies", import_settings.import_object_hierarchy);

        if(ImGui::Button("Save")) {
            AssetRegistry::save_metadata_for_asset(mesh_path, metadata);
        }

        ImGui::SameLine();

        if(ImGui::Button("Import")) {
            import_scene();
        }
    }

    void SceneImportWindow::import_scene() const {
        AssetRegistry::save_metadata_for_asset(mesh_path, metadata);

        auto registry = engine::g_engine->get_global_registry().lock();

        const auto scene_entity = importer->import_gltf_scene(mesh_path, metadata.import_settings, *registry);
        if(scene_entity) {
            const auto all_jsons = serialization::entity_and_children_to_json(*scene_entity, *registry);

            const nlohmann::json asset_json{{"entities", all_jsons}};
            const auto asset_json_string = nlohmann::to_string(asset_json);

            const auto dotpos = mesh_path.find_last_of(".");
            const auto mesh_path_no_extension = (dotpos == Rx::String::k_npos) ? mesh_path.substring(0, dotpos) : mesh_path;

            const auto asset_filename = Rx::String::format("%s.meshasset", mesh_path_no_extension);
            Rx::Filesystem::File asset_file{asset_filename, "w"};
            if(!asset_file.is_valid()) {
                logger->error("Could not open imported asset file %s", asset_filename);
                return;
            }

            asset_file.print(asset_json_string.c_str());
        }
    }
} // namespace sanity::editor::ui
