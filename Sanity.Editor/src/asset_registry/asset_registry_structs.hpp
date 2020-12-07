#pragma once

#include "core/types.hpp"

namespace sanity::editor {
    struct SceneImportSettings {
        bool import_meshes{true};
    	
        bool import_materials{true};

    	bool import_lights{true};

    	bool import_empties{true};

    	bool generate_collision_geometry{true};
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
