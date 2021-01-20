#pragma once

#include "core/types.hpp"
#include "rx/core/string.h"

namespace sanity::editor {
    struct SceneImportSettings {
        bool import_meshes{true};
    	
        float scaling_factor{1.0};
    	
        bool import_materials{true};

        bool generate_collision_geometry{true};

    	bool import_lights{true};

    	bool import_empties{true};

    	bool import_object_hierarchy{true};

       /*!
        * \brief Source file for this mesh asset
        *
        * Tells SanityEditor where to import this mesh from, if it needs reimporting
        */
        Rx::String source_file{};
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
