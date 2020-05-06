#pragma once

#include <cstdint>

#include "lights.hpp"

namespace renderer {
    struct StaticMeshRenderableComponent {
        uint32_t first_vertex{0};
        uint32_t num_vertices{0};

        uint32_t first_index{0};
        uint32_t num_indices{0};
    };

    struct CameraComponent {
        uint32_t idx;

        double fov{90};
        double aspect_ratio{9.0f / 16.0f};
        double near_clip_plane{0.01};
    };

    struct LightComponent {
        LightHandle handle;

        Light light;
    };
} // namespace renderer
