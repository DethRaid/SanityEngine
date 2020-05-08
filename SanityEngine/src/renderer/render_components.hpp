#pragma once

#include <cstdint>

#include "handles.hpp"
#include "lights.hpp"
#include "material_data_buffer.hpp"
#include "mesh.hpp"

namespace renderer {
    struct StaticMeshRenderableComponent {
        Mesh mesh;
        rhi::RaytracingMesh rt_mesh;

        MaterialHandle material{0};
    };

    struct PostProcessingPassComponent {
        uint32_t draw_idx{0};
        MaterialHandle material{0};
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
