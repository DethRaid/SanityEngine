#pragma once

#include "json5/json5.hpp"
#include "rx/core/string.h"

namespace environment {
    enum class FootprintClass { OneMeter, TwoMeters, FiveMeters, TenMeters };

    /*!
     * \brief An object which can be spawned by the procedural environment system
     *
     *
     */
    struct EnvironmentObject {
        /*!
         * \brief Approximate diameter of this object's footprint
         *
         * This puts environment objects into bins that make them easier to deal with
         *
         * Note that the footprint class does not have to be the actual size of the object. You can, for example, use a
         * small mesh with a large footprint to ensure that the objects are sparsely placed - or use a large mesh with
         * a small footprint to make objects that overlap
         */
        FootprintClass footprint_class{FootprintClass::TwoMeters};

        /*!
         * \brief The pipeline that generates this object's density map
         *
         * This will probably be stored in a much better way in the future, but we're not in the future
         */
        json5::value density_map_generation_pipeline{};

        /*!
         * \brief Path to this object's mesh, relative to the data directory
         *
         * TODO: Use GUIDs for asset references
         *
         * TODO: Store a reference to a full entity, not just a mesh
         */
        Rx::String mesh_file_path;
    };
} // namespace environment
