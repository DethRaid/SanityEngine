#pragma once

#include "core/GlmJsonConversion.hpp"
#include "core/transform.hpp"
#include "nlohmann/json.hpp"

namespace sanity::engine {
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Transform, location, rotation, scale);
}
