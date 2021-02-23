#pragma once

#include "core/types.hpp"
#include "entt/entity/registry.hpp"
#include "rx/core/map.h"
#include "rx/core/string.h"
#include "rx/core/utility/forward.h"

namespace Rx {
    struct String;
}

namespace sanity::engine {
    struct Transform;

    enum class ActorType {
        Default,
        FluidVolume,
    };

    /*!
     * \brief Component type for any entity within SanityEngine
     *
     * Entities have a system for sending and receiving events. Other components may subscribe to that system and react to events
     */
    struct __declspec(uuid("{6A611962-D937-4FC8-AF7D-7FFE4CD43749}")) Actor {
        Rx::String name;

        GUID id;

        Rx::Map<Rx::String, Int32> tags;

        entt::entity entity;

        entt::registry* registry;

        Rx::Vector<GUID> component_class_ids;

        void add_tag(const Rx::String& tag);

        void add_stacks_of_tag(const Rx::String& tag, Int32 num_stacks);

        void remove_tag(const Rx::String& tag);

        void remove_num_tags(const Rx::String& tag, Uint32 num_stacks);

        [[nodiscard]] Transform& get_transform() const;

        /*!
         * @brief Creates a component of the specified type in this actor
         *
         * @note Do not save the reference that this method returns. It will eventually become invalid as components are created and
         * destroyed
         *
         * @tparam ComponentType Type of the component to add
         * @tparam Args Types of the arguments to pass to the component's constructor
         * @param args Arguments to pass to the component's constructor
         *
         * @return A reference to the newly created component
         */
        template <typename ComponentType, typename... Args>
        ComponentType& add_component(Args&&... args);

        /**
         * @brief Adds a default instance of a component to this actor
         *
         * @param type_id ID of the type of the component
         */
        void add_component(GUID type_id);

        template <typename ComponentType>
        [[nodiscard]] bool has_component() const;

        [[nodiscard]] bool has_component(GUID guid) const;

        /*!
         * @brief Retrieves a reference to one of this actor's components
         *
         * @note Do not save the reference that this method returns. It will eventually become invalid as components are created and
         * destroyed
         *
         * @tparam ComponentType Type of the component to get
         *
         * @return A reference to the component
         */
        template <typename ComponentType>
        [[nodiscard]] ComponentType& get_component();
    };

    /*!
     * @brief Creates a new Actor in the provided registry
     *
     * @note Do not save the reference that this function returns. Instead, save the member `Actor::entity`. The returned reference will
     * eventually become invalid as actors get created and destroyed, but the entity ID is stable
     */
    [[nodiscard]] Actor& create_actor(entt::registry& registry, const Rx::String& name = "New Actor", ActorType actor_type = ActorType::Default);

    void draw_component_properties(Actor& entity);

    template <typename ComponentType, typename... Args>
    ComponentType& Actor::add_component(Args&&... args) {
        auto& component = registry->emplace<ComponentType>(entity, Rx::Utility::forward<Args>(args)...);

        component_class_ids.push_back(_uuidof(ComponentType));

        return component;
    }

    template <typename ComponentType>
    bool Actor::has_component() const {
        return registry->has<ComponentType>(entity);
    }

    template <typename ComponentType>
    ComponentType& Actor::get_component() {
        return registry->get<ComponentType>(entity);
    }
} // namespace sanity::engine
