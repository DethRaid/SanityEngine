#pragma once

#include "rhi/mesh_data_store.hpp"

namespace sanity::engine {
    struct BoundingBox {
        float x_min;
        float x_max;
        float y_min;
        float y_max;
        float z_min;
        float z_max;
    };

    namespace renderer {
        struct Mesh {
            Uint32 first_vertex{0};
            Uint32 num_vertices{0};

            Uint32 first_index{0};
            Uint32 num_indices{0};
        };

        struct MeshObject {
            Mesh mesh;

            BoundingBox bounds;
        };
    } // namespace renderer
} // namespace sanity::engine
