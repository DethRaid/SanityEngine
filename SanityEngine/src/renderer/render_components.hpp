#pragma once

#include "core/types.hpp"
#include "renderer/handles.hpp"
#include "renderer/lights.hpp"
#include "rhi/mesh_data_store.hpp"

namespace sanity::engine::renderer {
    /*!
     * \brief Renders a static mesh with some material
     */
    struct StandardRenderableComponent {
        /*!
         * \brief What type of object we're dealing with
         */
        enum class Type {
            /*!
             * \brief This object is in the foreground, and it's opaque. These objects should be drawn first
             */
            ForegroundOpaque = 100,

            /*!
             * \brief This object is in the background of the scene. These objects should be drawn after all opaque foreground objects
             */
            Background = 200,

            /*!
             * \brief This object is in the foreground, and it is transparent
             */
            ForegroundTransparent = 300,
        };

        /*!
         * \brief Mesh to render
         */
        Mesh mesh;

        /*!
         * \brief Material to use  when rendering that mesh
         */
        StandardMaterialHandle material{};

        /*!
         * \brief If true this object is ren
         */
        bool is_background{false};
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
