#include "EntitySerialization.hpp"

#include "actor/actor.hpp"
#include "core/components.hpp"
#include "entt/entity/registry.hpp"
#include "nlohmann/json.hpp"
#include "rx/core/vector.h"
#include "serialization/GetJsonForComponent.hpp"

// VS thinks this include is unused. VS is wrong
#include "core/RexJsonConversion.hpp"

namespace sanity::editor::serialization {
    nlohmann::json entity_to_json(const entt::entity& entity, const entt::registry& registry) {
        const auto& actor = registry.get<engine::Actor>(entity);

        // JSON representation of all the component on the entity
        Rx::Vector<nlohmann::json> component_jsons;

        actor.component_class_ids.each_fwd([&](const GUID& guid) {
            const auto& component_json = get_json_for_component(guid, entity, registry);
            component_jsons.push_back(component_json);
        });

        nlohmann::json json;
        json["components"] = component_jsons;

        return json;
    }

    Rx::Vector<nlohmann::json> entity_and_children_to_json(const entt::entity& entity, const entt::registry& registry) {
        Rx::Vector<nlohmann::json> jsons;

        auto entity_json = entity_to_json(entity, registry);

        if(const auto* transform_component = registry.try_get<engine::TransformComponent>(entity); transform_component != nullptr) {
            Rx::Vector<ENTT_ID_TYPE> child_entities;
            child_entities.reserve(transform_component->children.size());

            transform_component->children.each_fwd([&](const entt::entity& child_entity) {
                const auto child_jsons = entity_and_children_to_json(child_entity, registry);
                jsons.append(child_jsons);

                child_entities.push_back(static_cast<ENTT_ID_TYPE>(child_entity));

                // If the child entity doesn't have a SanityEngineEntity component for whatever reason, we can't save that it's one of our
                // children :(
            });

            entity_json.at("components").push_back(nlohmann::json{{"children", child_entities}});
        }

        jsons.push_back(entity_json);

        return jsons;
    }

    entt::entity json_to_entity(const nlohmann::json& json, entt::registry& registry) {
        const auto entity = registry.create();

        const auto& components = json.at("components");
        for(const auto& component : components) {
            create_component_from_json(component, entity, registry);
        }

        return entity;
    }
} // namespace sanity::editor::serialization
