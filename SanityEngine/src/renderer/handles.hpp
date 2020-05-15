#pragma once

#include <cstdint>

namespace renderer {
    struct TextureHandle {
        uint32_t index;
    };

    struct MaterialHandle {
        uint32_t index;
    };

    struct LightHandle {
        uint32_t index{0};
    };
} // namespace renderer
