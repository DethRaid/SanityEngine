#pragma once

namespace renderer {
    struct StaticMeshRenderable {
        uint32_t first_index{0};
        uint32_t num_indices{0};
    };

    struct CameraComponent {
        uint32_t idx;

        double fov{90};
        double aspect_ratio{};
        double near_clip_plane{0.01};
    };
} // namespace renderer
