#pragma once

#include <cstdint>

namespace renderer {
    struct ImageHandle {
        uint32_t handle;
    };

    struct LightHandle {
        uint32_t handle{0};
    };
} // namespace renderer
