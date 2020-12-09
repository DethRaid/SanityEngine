#pragma once

#include "handles.hpp"

namespace sanity::engine::renderer {
    /*!
     * \brief A standard SanityEngine physically-based material
     *
     * While this can not represent all the complexities of life, it can do a reasonable job at representing them
     *
     * If emission == 0 and metallic == 0, base color is the albedo
     * If emission == 0 and metallic == 1, base color is the specular color
     * If emission == 1, metallic no longer matters. Base color is the color of emitted light
     */
    struct StandardMaterial {
        TextureHandle base_color;

        TextureHandle normal;

    	/*!
    	 * G = roughness
    	 * B = metallic
    	 */
        TextureHandle metallic_roughness;
    	
        /*!
         * Emission is stored as the cube of the actual emission, scaled from 0 - 100 to 0 - 1
         *
         * `emission = pow(emission.r, 1 / 3) * 100;`
         */
    	TextureHandle emission;
    };
} // namespace renderer
