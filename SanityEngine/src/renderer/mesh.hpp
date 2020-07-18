#pragma once

#include "rhi/mesh_data_store.hpp"

struct BoundingBox {
    float x_min;
    float x_max;
    float y_min;
    float y_max;
    float z_min;
    float z_max;
};

namespace renderer {
    struct MeshObject {
        Mesh mesh;

        BoundingBox bounds;
    };
} // namespace renderer
