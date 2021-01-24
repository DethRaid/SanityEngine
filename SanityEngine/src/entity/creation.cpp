#include "creation.hpp"

#include "core/components.hpp"
#include "entt/entity/registry.hpp"

namespace sanity::engine {
    entt::entity create_entity(entt::registry& registry, const Rx::String& name) {
        const auto entity = registry.create();

        auto& sanity_entity = registry.emplace<SanityEngineEntity>(entity);
        sanity_entity.name = name;

        registry.emplace<TransformComponent>(entity);

    	return entity;
    }
} // namespace sanity::engine
