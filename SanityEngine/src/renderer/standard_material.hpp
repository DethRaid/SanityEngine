#pragma once

#include "handles.hpp"

namespace sanity::engine::renderer {
    struct StandardMaterial {
        /*!
         * \brief Handle to the texture with albedo in the rgb and transparency in the a
         */
        TextureHandle albedo;

        /*!
         * \brief Handle to a texture with normals in the rgb and roughness in the a
         */
        TextureHandle normal_roughness;

        /*!
         * \brief Handle to a texture with specular color in the rgb and emission strength in the a
         *
         * Emission is stored as the cube of the actual emission, scaled from 0 - 100 to 0 - 1
         *
         * `emission = pow(specular_color_emission.a, 1 / 3) * 100;`
         */
        TextureHandle specular_color_emission;

        /*!
         * \brief Noise texture
         */
        TextureHandle noise;
    };
} // namespace renderer
