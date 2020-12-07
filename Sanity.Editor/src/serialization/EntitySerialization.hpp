#pragma once

#include "entt/entity/fwd.hpp"
#include "nlohmann/json_fwd.hpp"
#include "rx/core/vector.h"

namespace sanity::editor::serialization {
    [[nodiscard]] nlohmann::json entity_to_json(const entt::entity& entity, const entt::registry& registry);

    [[nodiscard]] Rx::Vector<nlohmann::json> entity_and_children_to_json(const entt::entity& entity, const entt::registry& registry);

    [[nodiscard]] entt::entity json_to_entity(const nlohmann::json& json, entt::registry& registry);
} // namespace sanity::editor::serialization
