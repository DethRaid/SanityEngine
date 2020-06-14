#pragma once

#include <rx/core/ptr.h>

#include "renderer/handles.hpp"
#include "resources.hpp"

namespace rhi {
    constexpr Uint32 OPAQUE_OBJECT_BIT = 0x01;
    constexpr Uint32 TRANSPARENT_OBJECT_BIT = 0x02;
    constexpr Uint32 LIGHT_SOURCE_BIT = 0x10;

    /*!
     * \brief Represents an object that can be raytraced against
     *
     * In general, you should create one of these for each of the meshes in the scene
     */
    struct RaytracableGeometry {
        /*!
         * \brief Buffer that holds the bottom-level acceleration structure
         */
        Rx::Ptr<Buffer> blas_buffer;
    };

    struct RaytracingMaterial {
        Uint32 handle : 24;
    };

    struct RaytracingObject {
        /*!
         * \brief Buffer that holds the object's bottom-level acceleration structure
         */
        renderer::RaytracableGeometryHandle geometry_handle{0};

        /*!
         * \brief Material to render this RaytracingObject with
         */
        RaytracingMaterial material{0};
    };

    /*!
     * \brief Struct for the top level acceleration structure that we can raytrace against
     */
    struct RaytracingScene {
        Rx::Ptr<Buffer> buffer;
    };
} // namespace rhi
