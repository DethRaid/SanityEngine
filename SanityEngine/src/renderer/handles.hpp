#pragma once

#include <rx/core/types.h>

namespace renderer {
    struct TextureHandle {
        Uint32 index{0};
    };

    struct StandardMaterialHandle {
        Uint32 index{0};
    };

    struct LightHandle {
        Uint32 index{0};
    };

    struct RaytracableGeometryHandle {
        Uint32 index{0};
    };

    struct ModelMatrixHandle {
        Uint32 index{0};
    };
} // namespace renderer
