#pragma once

#include <guiddef.h>

#include "entt/entity/fwd.hpp"
#include "nlohmann/json_fwd.hpp"

namespace sanity::editor::serialization {
    [[nodiscard]] nlohmann::json get_json_for_component(const GUID& guid, const entt::entity& entity, const entt::registry& registry);

	void create_component_from_json(const nlohmann::json& component_json, const entt::entity& entity, entt::registry& registry);
}
