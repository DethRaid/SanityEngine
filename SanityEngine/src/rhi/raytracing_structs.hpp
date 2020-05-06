#pragma once

#include <memory>

#include "resources.hpp"

namespace rhi {
    /*!
     * \brief Represents an object that can be raytraced against
     *
     * In general, you should create one of these for each of the meshes in the scene
     */
    struct RaytracingMesh {
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
         * \brief Pointer to the mesh that this RaytracingObject uses
         */
        RaytracingMesh* mesh{nullptr};

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
