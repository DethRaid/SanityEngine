#pragma once

#include "core/types.hpp"
#include "renderer/handles.hpp"
#include "renderer/lights.hpp"
#include "rhi/mesh_data_store.hpp"

namespace sanity::engine::renderer {
    /*!
     * \brief Renders a static mesh with some material
     */
    struct __declspec(uuid("{74AA51B6-38C8-4D49-8A3C-C03BD56E2020}")) StandardRenderableComponent {
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
    struct __declspec(uuid("{3F869FC4-F339-4125-82F2-0A3775552112}")) PostProcessingPassComponent {
        Uint32 draw_idx{0};
        StandardMaterialHandle material{0};
    };

    /*!
     * \brief Sets up a camera to render with
     */
    struct __declspec(uuid("{23C1D6E0-B8E4-453A-8613-FE2EA86D2631}")) CameraComponent {
        Uint32 idx;

        double fov{75};
        double aspect_ratio{16.0 / 9.0};
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
    struct __declspec(uuid("{C1299481-3F19-4068-9724-FD89FF59EA65}")) LightComponent {
        LightHandle handle;

        Light light;
    };

    /*!
     * \brief Renders at atmospheric sky
     *
     * NOTE: Only one allowed in the scene ever
     */
    struct __declspec(uuid("{31AB3022-C3A9-4E48-AC49-2703C66A91EA}")) AtmosphericSkyComponent {};
} // namespace sanity::engine::renderer
