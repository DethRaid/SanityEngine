#pragma once

#include "asset_registry/asset_registry_structs.hpp"
#include "nlohmann/json.hpp"

namespace sanity::editor {
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        SceneImportSettings, import_meshes, import_materials, import_lights, import_empties, generate_collision_geometry);

    template <typename ImportSettingsType>
    void from_json(const nlohmann::json& j, AssetMetadata<ImportSettingsType>& asset_metadata) {
        j.at("last_import_date").get_to(asset_metadata.last_import_date);
        j.at("import_settings").get_to(asset_metadata.import_settings);
    }

    template <typename ImportSettingsType>
    void to_json(nlohmann::json& j, const AssetMetadata<ImportSettingsType>& asset_metadata) {
        j = nlohmann::json{{"last_import_date", asset_metadata.last_import_date}, {"import_settings", asset_metadata.import_settings}};
    }
} // namespace sanity::editor
