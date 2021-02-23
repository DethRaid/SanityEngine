#include "actor.hpp"

#include "core/components.hpp"
#include "ui/property_drawers.hpp"

namespace sanity::engine {
    void Actor::add_tag(const Rx::String& tag) {
        if(auto* num_stacks = tags.find(tag); num_stacks != nullptr) {
            (*num_stacks)++;
        } else {
            tags.insert(tag, 1);
        }
    }

    void Actor::add_stacks_of_tag(const Rx::String& tag, const Int32 num_stacks) {
        if(auto* cur_num_stacks = tags.find(tag); cur_num_stacks != nullptr) {
            (*cur_num_stacks) = num_stacks;
        } else {
            tags.insert(tag, num_stacks);
        }
    }

    void Actor::remove_tag(const Rx::String& tag) {
        if(auto* num_stacks = tags.find(tag); num_stacks != nullptr) {
            (*num_stacks)--;
        }
    }

    void Actor::remove_num_tags(const Rx::String& tag, const Uint32 num_stacks) {
        if(auto* cur_num_stacks = tags.find(tag); cur_num_stacks != nullptr) {
            (*cur_num_stacks) -= num_stacks;
        }
    }

    Transform& Actor::get_transform() const {
        auto& transform_component = registry->get<TransformComponent>(entity);
        return transform_component.transform;
    }

    Actor& create_actor(entt::registry& registry, const Rx::String& name) {
        const auto entity = registry.create();

        auto& actor_component = registry.emplace<Actor>(entity);
        actor_component.registry = &registry;
        actor_component.entity = entity;
        actor_component.name = name;

        actor_component.add_component<TransformComponent>();

        return actor_component;
    }

    void draw_component_properties(Actor& entity) {
        ui::draw_property("name", entity.name);
        ui::draw_property("tags", entity.tags);
    }

} // namespace sanity::engine
