#pragma once

#include <cstdint>

#include "handles.hpp"
#include "lights.hpp"
#include "material_data_buffer.hpp"
#include "mesh.hpp"

namespace renderer {
    /*!
     * \brief Renders a static mesh with some material
     */
    struct StaticMeshRenderableComponent {
        Mesh mesh;
        rhi::RaytracingMesh rt_mesh;

        MaterialHandle material{0};
    };

    /*!
     * \brief Renders a postprocessing pass
     */
    struct PostProcessingPassComponent {
        uint32_t draw_idx{0};
        MaterialHandle material{0};
    };

    /*!
     * \brief Sets up a camera to render with
     */
    struct CameraComponent {
        uint32_t idx;

        double fov{90};
        double aspect_ratio{9.0f / 16.0f};
        double near_clip_plane{0.01};
    };

    /*!
     * \brief A light that can illuminate the scene
     */
    struct LightComponent {
        LightHandle handle;

        Light light;
    };

    /*!
     * \brief Renders at atmospheric sky
     *
     * NOTE: Only one allowed in the scene ever
     */
    struct AtmosphericSkyComponent {
        MaterialHandle material{0};
    };
} // namespace renderer
