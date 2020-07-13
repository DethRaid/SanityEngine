#include "asset_registry.hpp"

#include <filesystem>

#include "rx/core/log.h"

RX_LOG("AssetRegistry", logger);

AssetRegistry::AssetRegistry(Rx::String content_directory_in) : content_directory{Rx::Utility::move(content_directory_in)} {
    register_assets_in_directory(std::filesystem::path{content_directory_in.data()});
}

Rx::Optional<Rx::String> AssetRegistry::get_path_from_guid(GUID guid) const {
    const auto* path_ptr = guid_to_file_path.find(guid);
    if(path_ptr == nullptr) {
        return Rx::nullopt;
    }

    return *path_ptr;
}

void AssetRegistry::register_assets_in_directory(const std::filesystem::path& directory_to_scan) {
    const auto directory_to_scan_string = directory_to_scan.string();
    logger->verbose("Scanning directory %s for assets", directory_to_scan.c_str());
    for(const auto& potential_asset_path : directory_to_scan) {
        const auto potential_asset_path_string = potential_asset_path.string();
        logger->verbose("Examining potential asset %s", potential_asset_path_string.c_str());

        if(is_directory(potential_asset_path)) {
            // Recurse into directories
            register_assets_in_directory(potential_asset_path);

        } else if(potential_asset_path.has_extension() && potential_asset_path.extension() != "meta") {
            // We have a file that isn't a meta file! Does it already have a .meta file?
            const auto meta_file_path_string = Rx::String::format("%s.meta", potential_asset_path_string.c_str());
            const auto meta_file_path = std::filesystem::path{meta_file_path_string.data()};

            if(!exists(meta_file_path)) {
                // Generate the meta file if it doesn't already exist
                generate_meta_file_for_asset(potential_asset_path);
            }

            register_asset_from_meta_file(meta_file_path);
        }
    }
}
