#pragma once

#include <Windows.h>

#include "rx/core/filesystem/directory.h"
#include "rx/core/map.h"
#include "rx/core/optional.h"
#include "rx/core/string.h"
#include "serialization/serialization.hpp"
#include "types.hpp"

namespace std {
    namespace filesystem {
        // ReSharper disable once CppInconsistentNaming
        class path;
    } // namespace filesystem
} // namespace std

struct AssetMetadataFile {
    int version{1};
    GUID guid;
};

JSON5_CLASS(AssetMetadataFile, version, guid)

constexpr int METADATA_CURRENT_VERSION = 1;

/*!
 * \brief Stores references to all the assets and how awesome they all are
 */
class AssetRegistry {
public:
    /*!
     * \brief Creates a new AssetRegistry instance
     *
     * The new AssetRegistry will scan the given directory and all subdirectories for `.meta` files. The AssetRegistry reads the GUIDs in
     * all those files and builds a map from an asset's GUID to its location on the filesystem. Then, when you actually open an asset, the
     * asset viewer will query this registry for the file paths of all the referenced assets and load them as needed
     */
    explicit AssetRegistry(const Rx::String& content_directory_in);

    /*!
     * \brief Retrieves the location on the filesystem for the asset with the provided GUID
     */
    [[nodiscard]] Rx::Optional<Rx::String> get_path_from_guid(GUID guid) const;

private:
    Rx::Filesystem::Directory content_directory;

    Rx::Map<GUID, Rx::String> guid_to_file_path;

    /*!
     * \brief Registers all the assets in the provided directory with the asset registry
     *
     * If the assets doesn't have a .meta file, this method generates one
     *
     * \param directory_to_scan The directory to look in for assets
     */
    void register_assets_in_directory(Rx::Filesystem::Directory& directory_to_scan);

    /*!
     * \brief Registers the asset described by the provided meta file
     */
    void register_asset_from_meta_data(const AssetMetadataFile& meta_data);

    /*!
     * \brief Generates the meta file for the asset at the provided path, overwriting any existing meta file
     *
     * This method does not register the asset with the asset registry
     */
    void generate_meta_file_for_asset(const Rx::Filesystem::Directory& asset_path);
};
