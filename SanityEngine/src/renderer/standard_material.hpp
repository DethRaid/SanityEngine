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
	    glm::vec4 base_color_value{0.8f, 0.8f, 0.8f, 0.f};
        
        glm::vec4 metallic_roughness_value{0.f, 0.f, 0.5f, 0.f};

    	glm::vec4 emission_value{0.f};
    	
        TextureHandle base_color_texture{0};

        TextureHandle normal_texture{0};

    	/*!
    	 * G = roughness
    	 * B = metallic
    	 */
        TextureHandle metallic_roughness_texture{0};
    	
        /*!
         * Emission is stored as the cube of the actual emission, scaled from 0 - 100 to 0 - 1
         *
         * `emission = pow(emission.r, 1 / 3) * 100;`
         */
        TextureHandle emission_texture{0};
    };
} // namespace renderer
