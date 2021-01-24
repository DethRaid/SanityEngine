#include "entity_operations.hpp"

#include "core/components.hpp"
#include "entity/Components.hpp"
#include "entt/entity/registry.hpp"

namespace sanity::editor::entity {
    entt::entity create_base_editor_entity(const Rx::String& name, entt::registry& registry) {
        const auto new_entity = registry.create();

        registry.emplace<ComponentClassIdList>(new_entity);

        auto& entity_component = add_component<engine::SanityEngineEntity>(new_entity, registry);
        if(!name.is_empty()) {
            entity_component.name = name;
        	
        } else {
            entity_component.name = "New Entity";
        }

        return new_entity;
    }
} // namespace sanity::editor::entity
