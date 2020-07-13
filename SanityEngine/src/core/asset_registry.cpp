#include "asset_registry.hpp"

#include <filesystem>

#include <combaseapi.h>

#include "rx/core/log.h"

RX_LOG("AssetRegistry", logger);

AssetRegistry::AssetRegistry(const Rx::String& content_directory_in) : content_directory{content_directory_in} {
    register_assets_in_directory(content_directory);
}

Rx::Optional<Rx::String> AssetRegistry::get_path_from_guid(GUID guid) const {
    const auto* path_ptr = guid_to_file_path.find(guid);
    if(path_ptr == nullptr) {
        return Rx::nullopt;
    }

    return *path_ptr;
}

void AssetRegistry::register_assets_in_directory(Rx::Filesystem::Directory& directory_to_scan) {
    logger->verbose("Scanning directory %s for assets", directory_to_scan.path());
    directory_to_scan.each([&](Rx::Filesystem::Directory::Item& potential_asset_path) {
        logger->verbose("Examining potential asset %s", potential_asset_path.name());

        if(potential_asset_path.is_directory()) {
            // Recurse into directories
            auto child_directory = Rx::Filesystem::Directory{potential_asset_path.name()};
            register_assets_in_directory(child_directory);

        } else if(!potential_asset_path.name().ends_with("meta")) {
            // We have a file that isn't a meta file! Does it already have a .meta file?
            const auto meta_file_path_string = Rx::String::format("%s.meta", potential_asset_path.name());
            const auto meta_file_path = std::filesystem::path{meta_file_path_string.data()};

            if(!exists(meta_file_path)) {
                // Generate the meta file if it doesn't already exist
                generate_meta_file_for_asset(potential_asset_path.name());
            }


            AssetMetadataFile metadata;
            json5::from_file(meta_file_path_string.data(), metadata);
            register_asset_from_meta_data(metadata);
        }
    });
}

void AssetRegistry::register_asset_from_meta_data(const AssetMetadataFile& meta_data)
{
    
}

void AssetRegistry::generate_meta_file_for_asset(const Rx::Filesystem::Directory& asset_path) {
    GUID guid;
    const auto result = CoCreateGuid(&guid);
    if(FAILED(result)) {
        logger->error("Could not create GUID for file %s", asset_path.path());
        return;
    }

    const auto meta_file_path = Rx::String::format("%s.meta", asset_path.path());

    const auto meta_data = AssetMetadataFile{.version = METADATA_CURRENT_VERSION, .guid = guid};
    json5::to_file(meta_file_path.data(), meta_data);
}
