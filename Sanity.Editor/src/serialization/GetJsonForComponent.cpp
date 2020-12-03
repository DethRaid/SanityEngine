#include "GetJsonForComponent.hpp"

#include "core/ComponentJsonConversion.hpp"
#include "entt/entity/registry.hpp""

namespace sanity::editor::serialization {
    nlohmann::json get_json_for_component(const GUID& guid, const entt::entity& entity, const entt::registry& registry) {
        nlohmann::json json;

        if(guid == __uuidof(TransformComponent)) {
            const auto& component = registry.get<TransformComponent>(entity);
            to_json(json, component);
        }

        return json;
    }
} // namespace sanity::editor::serialization
