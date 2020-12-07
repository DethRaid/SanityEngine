#include "GetJsonForComponent.hpp"

#include "core/ComponentJsonConversion.hpp"
#include "core/WindowsJsonConversion.hpp"
#include "entt/entity/registry.hpp"

namespace sanity::editor::serialization {
    nlohmann::json get_json_for_component(const GUID& guid, const entt::entity& entity, const entt::registry& registry) {
        nlohmann::json json;

        if(guid == __uuidof(engine::TransformComponent)) {
            const auto& component = registry.get<engine::TransformComponent>(entity);
            to_json(json, component);

        } else if(guid == _uuidof(engine::SanityEngineEntity)) {
            const auto& component = registry.get<engine::SanityEngineEntity>(entity);
            to_json(json, component);
        }

        return json;
    }

    void create_component_from_json(const nlohmann::json& component_json, const entt::entity& entity, entt::registry& registry) {
        const auto& class_id = component_json.at("_class_id").get<GUID>();

        if(class_id == __uuidof(engine::TransformComponent)) {
            auto& component = registry.emplace<engine::TransformComponent>(entity);
            component_json.get_to(component);
        	
        } else if(class_id == __uuidof(engine::SanityEngineEntity)) {
            auto& component = registry.emplace<engine::SanityEngineEntity>(entity);
            component_json.get_to(component);
        }
    }
} // namespace sanity::editor::serialization
