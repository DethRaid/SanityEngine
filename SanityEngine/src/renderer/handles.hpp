#pragma once

#include <cstdint>

namespace renderer {
    struct TextureHandle {
        uint32_t index{0};
    };

    struct StandardMaterialHandle {
        uint32_t index{0};
    };

    struct LightHandle {
        uint32_t index{0};
    };

    struct RaytracableGeometryHandle {
        uint32_t index{0};
    };
} // namespace renderer
