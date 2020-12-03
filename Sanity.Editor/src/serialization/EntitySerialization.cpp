#include "EntitySerialization.hpp"

#include "GetJsonForComponent.hpp"
#include "core/RexJsonConversion.hpp"
#include "entity/Components.hpp"
#include "entt/entity/entity.hpp"
#include "entt/entity/registry.hpp"
#include "nlohmann/json.hpp"
#include "rex/include/rx/core/vector.h"

namespace sanity::editor::serialization {
    nlohmann::json entity_to_json(const entt::entity& entity, const entt::registry& registry) {
        const auto& component_list = registry.get<ComponentClassIdList>(entity);

        // JSON representation of all the component on the entity
        Rx::Vector<nlohmann::json> component_jsons;

        component_list.class_ids.each_fwd([&](const GUID& guid) {
            const auto& component_json = get_json_for_component(guid, entity, registry);
            component_jsons.push_back(component_json);
        });

        nlohmann::json json;
        json["components"] = component_jsons;

        return json;
    }
} // namespace sanity::editor::serialization
