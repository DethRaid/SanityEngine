#pragma once

#include "core/types.hpp"

namespace sanity::engine::renderer {
    struct Handle {
        Uint32 index{0xFFFFFFFF};

        [[nodiscard]] bool operator==(const Handle& other) const = default;

        [[nodiscard]] bool is_valid() const;
    };

    struct BufferHandle : Handle {};

    struct TextureHandle : Handle {};

    struct FluidVolumeHandle : Handle {};

    struct StandardMaterialHandle : Handle {};

    struct LightHandle : Handle {};

    struct RaytracingAsHandle : Handle {};

    struct ModelMatrixHandle : Handle {};

    inline bool Handle::is_valid() const { return index != 0xFFFFFFFF; }
} // namespace sanity::engine::renderer
