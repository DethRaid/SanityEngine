#include "GetJsonForComponent.hpp"

#include "core/ComponentJsonConversion.hpp"
#include "core/WindowsJsonConversion.hpp"
#include "entt/entity/registry.hpp"

namespace sanity::editor::serialization {
    nlohmann::json get_json_for_component(const GUID& guid, const entt::entity& entity, const entt::registry& registry) {
        nlohmann::json json;

        if(guid == __uuidof(TransformComponent)) {
            const auto& component = registry.get<TransformComponent>(entity);
            to_json(json, component);
        }

        return json;
    }

    void create_component_from_json(const nlohmann::json& component, const entt::entity& entity, entt::registry& registry) {
        const auto& class_id = component.at("_class_id").get<GUID>();

        if(class_id == __uuidof(TransformComponent)) {
            auto& transform = registry.emplace<TransformComponent>(entity);
            component.get_to(transform);
        }
    }
} // namespace sanity::editor::serialization
