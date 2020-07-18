#include "asset_registry.hpp"

#include <filesystem>

#include <combaseapi.h>

#include "rx/core/filesystem/file.h"
#include "rx/core/log.h"
#include "sanity_engine.hpp"

RX_LOG("AssetRegistry", logger);

AssetRegistry::AssetRegistry(const Rx::String& content_directory_in)
    : content_directory{Rx::String::format("%s/%s", SanityEngine::executable_directory, content_directory_in)} {
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

        auto child_directory = potential_asset_path.as_directory();
        if(child_directory) {
            // Recurse into directories
            register_assets_in_directory(*child_directory);

        } else if(!potential_asset_path.name().ends_with("meta")) {
            const auto full_file_path = Rx::String::format("%s/%s", directory_to_scan.path(), potential_asset_path.name());

            // We have a file that isn't a meta file! Does it already have a .meta file?
            const auto meta_file_path = Rx::String::format("%s.meta", full_file_path);

            logger->verbose("Attempting to read meta file %s", meta_file_path);

            auto meta_file_contents = Rx::Filesystem::read_text_file(meta_file_path);
            if(!meta_file_contents) {
                generate_meta_file_for_asset(full_file_path);
                meta_file_contents = Rx::Filesystem::read_text_file(meta_file_path);
            }

            AssetMetadataFile metadata;
            json5::from_string(std::string{reinterpret_cast<const char*>(meta_file_contents->data())}, metadata);

            register_asset_from_meta_data(metadata);
        }
    });
}

void AssetRegistry::register_asset_from_meta_data(const AssetMetadataFile& meta_data) {}

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

    logger->verbose("Created meta file %s", meta_file_path);
}
