#pragma once

#include "core/types.hpp"

namespace sanity::engine::renderer {
    struct TextureHandle {
        Uint32 index{0};

    	bool operator==(const TextureHandle& other) const = default;
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

