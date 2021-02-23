#include "actor.hpp"

#include "core/components.hpp"
#include "renderer/render_components.hpp"
#include "rx/core/log.h"
#include "sanity_engine.hpp"
#include "ui/property_drawers.hpp"

namespace sanity::engine {
    RX_LOG("Actor", logger);

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

    void Actor::add_component(GUID type_id) {
        if(type_id == __uuidof(TransformComponent)) {
            add_component<TransformComponent>();

        } else if(type_id == __uuidof(renderer::StandardRenderableComponent)) {
            add_component<renderer::StandardRenderableComponent>();

        } else if(type_id == __uuidof(renderer::PostProcessingPassComponent)) {
            add_component<renderer::PostProcessingPassComponent>();

        } else if(type_id == __uuidof(renderer::RaytracingObjectComponent)) {
            add_component<renderer::RaytracingObjectComponent>();

        } else if(type_id == __uuidof(renderer::CameraComponent)) {
            add_component<renderer::CameraComponent>();

        } else if(type_id == __uuidof(renderer::LightComponent)) {
            add_component<renderer::LightComponent>();

        } else if(type_id == __uuidof(renderer::SkyComponent)) {
            add_component<renderer::SkyComponent>();

        } else if(type_id == __uuidof(renderer::FluidVolumeComponent)) {
            add_component<renderer::FluidVolumeComponent>();

        } else {
            logger->error("%s: Unknown component type %s, unable to add", __func__, type_id);
            DebugBreak();
        }
    }

    bool Actor::has_component(const GUID guid) const {
        if(guid == __uuidof(TransformComponent)) {
            return has_component<TransformComponent>();

        } else if(guid == __uuidof(renderer::StandardRenderableComponent)) {
            return has_component<renderer::StandardRenderableComponent>();

        } else if(guid == __uuidof(renderer::PostProcessingPassComponent)) {
            return has_component<renderer::PostProcessingPassComponent>();

        } else if(guid == __uuidof(renderer::RaytracingObjectComponent)) {
            return has_component<renderer::RaytracingObjectComponent>();

        } else if(guid == __uuidof(renderer::CameraComponent)) {
            return has_component<renderer::CameraComponent>();

        } else if(guid == __uuidof(renderer::LightComponent)) {
            return has_component<renderer::LightComponent>();

        } else if(guid == __uuidof(renderer::SkyComponent)) {
            return has_component<renderer::SkyComponent>();

        } else if(guid == __uuidof(renderer::FluidVolumeComponent)) {
            return has_component<renderer::FluidVolumeComponent>();
        }

        // Ignore some components that aren't user-facing
        // TODO: A smart way to disambiguate user-facing components from internal ones

        return false;
    }

    Actor& create_actor(entt::registry& registry, const Rx::String& name, const ActorType actor_type) {
        const auto entity = registry.create();

        auto& actor = registry.emplace<Actor>(entity);
        actor.registry = &registry;
        actor.entity = entity;
        actor.name = name;

        actor.add_component<TransformComponent>();

        if(actor_type == ActorType::FluidVolume) {
            auto& volume = actor.add_component<renderer::FluidVolumeComponent>();

            auto& renderer = g_engine->get_renderer();

            const auto fluid_info = renderer::FluidVolumeCreateInfo{.name = name, .size = {10, 10, 10}};
            volume.volume = renderer.create_fluid_volume(fluid_info);
        }

        return actor;
    }

    void draw_component_properties(Actor& entity) {
        ui::draw_property("name", entity.name);
        ui::draw_property("tags", entity.tags);
    }

} // namespace sanity::engine
