#pragma once

#include "core/types.hpp"

namespace sanity::engine::renderer {
    struct TextureHandle {
        Uint32 index{0};

    	[[nodiscard]] bool operator==(const TextureHandle& other) const = default;

    	[[nodiscard]] bool is_valid() const;
    };
    
    struct StandardMaterialHandle {
        Uint32 index{0};
    };

    struct LightHandle {
        Uint32 index{0};
    };

    struct RaytracingASHandle {
        Uint32 index{0};
    };

    struct ModelMatrixHandle {
        Uint32 index{0};
    };

    inline bool TextureHandle::is_valid() const { return index != 0; }
} // namespace renderer

