#pragma once

#include "core/GlmJsonConversion.hpp"
#include "core/JsonConversion.hpp"
#include "core/RexJsonConversion.hpp"
#include "core/WindowsJsonConversion.hpp"
#include "core/components.hpp"
#include "core/json/transform_json_conversion.hpp"

namespace sanity::engine {
    SANITY_DEFINE_COMPONENT_JSON_CONVERSIONS(TransformComponent, transform);
    SANITY_DEFINE_COMPONENT_JSON_CONVERSIONS(Actor, name, id, tags);
} // namespace sanity::engine
