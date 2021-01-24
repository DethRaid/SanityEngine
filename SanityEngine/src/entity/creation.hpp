#pragma once

#include "entt/entity/fwd.hpp"
#include "rx/core/string.h"

namespace sanity::engine {
    [[nodiscard]] entt::entity create_entity(entt::registry& registry, const Rx::String& name = "New entity");
}
