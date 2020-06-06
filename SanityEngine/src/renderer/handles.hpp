#pragma once

#include <cstdint>

namespace renderer {
    struct TextureHandle {
        size_t index{0};
    };

    struct MaterialHandle {
        uint32_t index{0};
    };

    struct LightHandle {
        size_t index{0};
    };

    struct RaytracableGeometryHandle {
        size_t index{0};
    };
} // namespace renderer
