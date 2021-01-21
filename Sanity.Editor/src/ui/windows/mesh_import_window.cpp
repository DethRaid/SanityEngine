#include "mesh_import_window.hpp"

#include "SanityEditor.hpp"
#include "core/RexJsonConversion.hpp"
#include "import/scene_importer.hpp"
#include "rx/core/log.h"
#include "sanity_engine.hpp"
#include "serialization/EntitySerialization.hpp"
#include "ui/property_drawers.hpp"

using namespace sanity::engine::ui;

namespace sanity::editor::ui {
    RX_LOG("SceneImportWindow", logger);
    SceneImportWindow::SceneImportWindow(const std::filesystem::path& mesh_path_in)
        : metadata{AssetRegistry::get_meta_for_asset<SceneImportSettings>(mesh_path_in)}, mesh_path{mesh_path_in} {
        name = Rx::String::format("Import %s", mesh_path_in.filename());

        auto& renderer = engine::g_engine->get_renderer();
        importer = Rx::make_ptr<import::SceneImporter>(RX_SYSTEM_ALLOCATOR, renderer);
    }

    void SceneImportWindow::draw_contents() {
        auto& import_settings = metadata.import_settings;

        draw_property_editor("Import meshes", import_settings.import_meshes);
        draw_property_editor("Scaling factor", import_settings.scaling_factor);
        draw_property_editor("Import materials", import_settings.import_materials);
        draw_property_editor("Import lights", import_settings.import_lights);
        draw_property_editor("Import entities", import_settings.import_empties);
        draw_property_editor("Import object hierarchies", import_settings.import_object_hierarchy);

        // Intentionally not drawing a property editor for source_file - source_file gets set automatically when you
        // import a mesh

        if(ImGui::Button("Save")) {
            AssetRegistry::save_meta_for_asset(mesh_path, metadata);
        }

        ImGui::SameLine();

        if(ImGui::Button("Import")) {        	
            import_scene();
        }
    }

    void SceneImportWindow::import_scene() const {
        AssetRegistry::save_meta_for_asset(mesh_path, metadata);

        auto registry = engine::g_engine->get_global_registry().lock();

        const auto scene_entity = importer->import_gltf_scene(mesh_path, metadata.import_settings, *registry);
        if(scene_entity) {
        }
    }
} // namespace sanity::editor::ui
