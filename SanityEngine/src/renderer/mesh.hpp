#pragma once

#include "core/types.hpp"

namespace sanity::engine {
    struct BoundingBox {
        float x_min{0};
        float x_max{0};
        float y_min{0};
        float y_max{0};
        float z_min{0};
        float z_max{0};
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
