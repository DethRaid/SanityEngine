#include "asset_registry.hpp"

#include <filesystem>

#include <combaseapi.h>

#include "rx/core/filesystem/file.h"
#include "rx/core/log.h"
#include "sanity_engine.hpp"

RX_LOG("AssetRegistry", logger);

AssetRegistry::AssetRegistry(const Rx::String& content_directory_in)
    : content_directory{Rx::String::format("%s/%s", SanityEngine::executable_directory, content_directory_in)} {
}

Rx::Optional<Rx::String> AssetRegistry::get_path_from_guid(GUID guid) const {
    const auto* path_ptr = guid_to_file_path.find(guid);
    if(path_ptr == nullptr) {
        return Rx::nullopt;
    }

    return *path_ptr;
}
