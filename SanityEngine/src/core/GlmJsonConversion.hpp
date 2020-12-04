#pragma once

#include "core/JsonConversion.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

namespace glm {
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(vec2, x, y)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(vec3, x, y, z)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(vec4, x, y, z, w)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(quat, x, y, z, w)
}
