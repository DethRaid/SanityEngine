#pragma once

#include <cstdint>

namespace renderer {
    struct Mesh {
        uint32_t first_vertex{0};
        uint32_t num_vertices{0};

        uint32_t first_index{0};
        uint32_t num_indices{0};
    };
} // namespace renderer
