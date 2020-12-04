#pragma once

#include "core/types.hpp"

namespace sanity::editor {
    struct MeshImportSettings {
        bool import_materials{true};
    };

    /*!
     * \brief Generic asset import information, such as last modification time
     */
    template <typename AssetImportSettingsType>
    struct AssetMetadata {
        Uint64 last_import_date{0};

        AssetImportSettingsType import_settings{};
    };
} // namespace sanity::editor
