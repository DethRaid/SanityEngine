#pragma once

#include <memory>

#include "resources.hpp"

#include "renderer/handles.hpp"

namespace rhi {
    constexpr uint32_t OPAQUE_OBJECT_BIT = 0x01;
    constexpr uint32_t TRANSPARENT_OBJECT_BIT = 0x02;
    constexpr uint32_t LIGHT_SOURCE_BIT = 0x10;

    /*!
     * \brief Represents an object that can be raytraced against
     *
     * In general, you should create one of these for each of the meshes in the scene
     */
    struct RaytracableGeometry {
        /*!
         * \brief Buffer that holds the bottom-level acceleration structure
         */
        std::unique_ptr<Buffer> blas_buffer;
    };

    struct RaytracingMaterial {
        uint32_t handle : 24;
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
        std::unique_ptr<Buffer> buffer;
    };
} // namespace rhi
