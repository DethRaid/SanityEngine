#pragma once

#include "core/GlmJsonConversion.hpp"
#include "core/JsonConversion.hpp"
#include "core/RexJsonConversion.hpp"
#include "core/WindowsJsonConversion.hpp"
#include "core/components.hpp"

namespace sanity::engine {
    SANITY_DEFINE_COMPONENT_JSON_CONVERSIONS(TransformComponent, location, rotation, scale);
    SANITY_DEFINE_COMPONENT_JSON_CONVERSIONS(SanityEngineEntity, name, tags);
} // namespace sanity::engine
