#pragma once

#include <cstdint>

namespace renderer {
    struct ImageHandle {
        uint32_t idx;
    };

    struct LightHandle {
        uint32_t handle{0};
    };
} // namespace renderer
