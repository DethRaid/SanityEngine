#pragma once

#include <guiddef.h>

#include "entt/entity/fwd.hpp"
#include "nlohmann/json_fwd.hpp"

namespace sanity::editor::serialization {
    [[nodiscard]] nlohmann::json get_json_for_component(const GUID& guid, const entt::entity& entity, const entt::registry& registry);
}
