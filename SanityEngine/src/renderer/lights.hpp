#pragma once

#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"
#include "glm/vec3.hpp"

namespace sanity::engine::renderer {
    constexpr Uint32 MAX_NUM_LIGHTS = 32;

    enum class LightType {
        Directional = 0,
    };

    struct Light {
        LightType type{LightType::Directional};

        /*!
         * \brief HDR color of this light
         */
        glm::vec3 color{glm::normalize(glm::vec3{1, 1, 1}) * 22.0f};

        glm::vec3 direction{glm::normalize(glm::vec3{-1, 3, 1})};

        /*!
         * Angular size of the light, in radians. Only relevant for directional lights
         */
        Float32 angular_size{glm::radians(0.53f)};
        // Hack to make the soft shadows easier to see in my test scene, should remove the *10 multiplier when I have a real scene
    };
} // namespace renderer
