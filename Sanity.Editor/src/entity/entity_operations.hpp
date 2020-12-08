#pragma once

#include "entity/Components.hpp"
#include "entt/entity/entity.hpp"
#include "entt/entity/registry.hpp"

namespace Rx {
    struct String;
}

namespace sanity::editor::entity {
    [[nodiscard]] entt::entity create_base_editor_entity(const Rx::String& name, entt::registry& registry);

    template <typename ComponentType, typename... Args>
    ComponentType& add_component(const entt::entity& entity, entt::registry& registry, Args&&... args);

    template <typename ComponentType, typename... Args>
    ComponentType& add_component(const entt::entity& entity, entt::registry& registry, Args&&... args) {
        auto& component = registry.emplace<ComponentType>(entity, Rx::Utility::forward<Args>(args)...);

        auto& component_list = registry.get<ComponentClassIdList>(entity);
        component_list.class_ids.push_back(_uuidof(ComponentType));

        return component;
    }
} // namespace sanity::editor::entity
