#pragma once

#include <cstdint>

#include "rhi/mesh_data_store.hpp"
#include "renderer/handles.hpp"
#include "renderer/lights.hpp"

namespace renderer {
    /*!
     * \brief Renders a static mesh with some material
     */
    struct StandardRenderableComponent {
        rhi::Mesh mesh;

        StandardMaterialHandle material{};
    };

    /*!
     * \brief Component for a mesh in a chunk
     *
     * This will probably change a lot as time goes on, but for now this should work
     */
    struct ChunkMeshComponent {
        rhi::Mesh mesh;
    };

    /*!
     * \brief Renders a postprocessing pass
     */
    struct PostProcessingPassComponent {
        Uint32 draw_idx{0};
        StandardMaterialHandle material{0};
    };

    /*!
     * \brief Sets up a camera to render with
     */
    struct CameraComponent {
        Uint32 idx;

        double fov{90};
        double aspect_ratio{9.0f / 16.0f};
        double near_clip_plane{0.01};

        /*!
         * \brief Width of the camera frustum, in local space
         *
         * Only relevant if `fov` is 0
         */
        double orthographic_size = 100;
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
    struct AtmosphericSkyComponent {};
} // namespace renderer
