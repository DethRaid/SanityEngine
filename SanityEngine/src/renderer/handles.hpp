#pragma once

#include <cstdint>

namespace renderer {
    struct TextureHandle {
        uint32_t handle;
    };

    struct MaterialHandle {
        uint32_t handle;
    };

    struct LightHandle {
        uint32_t handle{0};
    };
} // namespace renderer
