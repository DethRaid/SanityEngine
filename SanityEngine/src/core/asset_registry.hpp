#pragma once

#include <Windows.h>

#include "core/Prelude.hpp"
#include "rx/core/filesystem/directory.h"
#include "rx/core/map.h"
#include "rx/core/optional.h"
#include "rx/core/string.h"
#include "types.hpp"

namespace std {
    namespace filesystem {
        // ReSharper disable once CppInconsistentNaming
        class path;
    } // namespace filesystem
} // namespace std

constexpr int METADATA_CURRENT_VERSION = 1;

/*!
 * \brief Stores references to all the assets and how awesome they all are
 */
class SANITY_API AssetRegistry {
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
};
