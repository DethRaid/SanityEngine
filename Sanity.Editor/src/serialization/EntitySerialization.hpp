#pragma once

#include "entt/entity/fwd.hpp"
#include "nlohmann/json_fwd.hpp"

namespace sanity::editor::serialization {
    [[nodiscard]] nlohmann::json entity_to_json(const entt::entity& entity, const entt::registry& registry);

    [[nodiscard]] entt::entity json_to_entity(const nlohmann::json& json, entt::registry& registry);
}
