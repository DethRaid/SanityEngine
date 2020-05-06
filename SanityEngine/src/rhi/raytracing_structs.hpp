#pragma once

#include <memory>

#include "resources.hpp"

namespace rhi {
    /*!
     * \brief Represents an object that can be raytraced against
     *
     * In general, you should create one of these for each of the meshes in the scene
     */
    struct Blas {
        /*!
         * \brief Buffer that holds the bottom-level acceleration structure
         */
        std::unique_ptr<Buffer> blas_buffer;
    };
}
