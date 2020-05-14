#pragma once

#include "handles.hpp"

struct StandardMaterial {
    /*!
     * \brief Handle to the texture with albedo in the rgb and transparency in the a
     */
    renderer::ImageHandle albedo;

    /*!
     * \brief Handle to a texture with normals in the rgb and roughness in the a
     */
    renderer::ImageHandle normal_roughness;

    /*!
     * \brief Handle to a texture with specular color in the rgb and emission strength in the a
     *
     * Emission is stored as the cube of the actual emission, scaled from 0 - 100 to 0 - 1
     *
     * `emission = pow(specular_color_emission.a, 1 / 3) * 100;`
     */
    renderer::ImageHandle specular_color_emission;

    /*!
     * \brief Noise texture
     */
    renderer::ImageHandle noise;
};
