#pragma once

#include "handles.hpp"
#include "renderer/rhi/resources.hpp"

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
    	
        GpuResourceHandle<Texture> base_color_texture{};

        GpuResourceHandle<Texture> normal_texture{};

    	/*!
    	 * G = roughness
    	 * B = metallic
    	 */
        GpuResourceHandle<Texture> metallic_roughness_texture{};
    	
        /*!
         * Emission is stored as the cube of the actual emission, scaled from 0 - 100 to 0 - 1
         *
         * `emission = pow(emission.r, 1 / 3) * 100;`
         */
        GpuResourceHandle<Texture> emission_texture{};
    };

	using StandardMaterialHandle = GpuResourceHandle<StandardMaterial>;
} // namespace renderer
