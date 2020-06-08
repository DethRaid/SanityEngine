#include "entity_scripting_api.hpp"

#include <wren/wren.hpp>

#include "../globals.hpp"
#include "../sanity_engine.hpp"
#include "../world/world.hpp"
#include "scripting_runtime.hpp"

namespace horus {
    Entity::Entity(WrenHandle* handle_in, const entt::entity entity_in, entt::registry& registry_in)
        : handle{handle_in}, entity{entity_in}, registry{registry_in} {}

    void Entity::add_tag(const std::string& tag) const {
        auto& tags = registry.get_or_assign<TagComponent>(entity);
        tags.tags.insert(tag);
    }

    bool Entity::has_tag(const std::string& tag) const {
        if(registry.has<TagComponent>(entity)) {
            const auto& tags = registry.get<TagComponent>(entity);
            return tags.tags.find(tag) != tags.tags.end();
        }

        return false;
    }

    std::unordered_set<std::string> Entity::get_tags() const {
        if(registry.has<TagComponent>(entity)) {
            const auto& tag_component = registry.get<TagComponent>(entity);
            return tag_component.tags;

        } else {
            return {};
        }
    }

    World* Entity::get_world() const { return g_engine->get_world(); }

    WrenHandle* Entity::get_handle() const { return handle; }

    Component::Component(const entt::entity entity_in, WrenHandle* handle_in, const ScriptComponentMethods& class_methods_in, WrenVM& vm_in)
        : entity{entity_in}, class_methods{class_methods_in}, component_handle{handle_in}, vm{&vm_in} {}

    void Component::initialize_self() {}

    void Component::begin_play(World& world) const {
        wrenEnsureSlots(vm, 2);
        wrenSetSlotHandle(vm, 0, component_handle);
        wrenSetSlotHandle(vm, 1, world._get_wren_handle());

        wrenCall(vm, class_methods.begin_play_handle);
    }

    void Component::tick(const float delta_seconds) const {
        wrenEnsureSlots(vm, 2);
        wrenSetSlotHandle(vm, 0, component_handle);
        wrenSetSlotDouble(vm, 1, delta_seconds);

        wrenCall(vm, class_methods.tick_handle);
    }

    void Component::end_play() const {
        wrenEnsureSlots(vm, 1);
        wrenSetSlotHandle(vm, 0, component_handle);

        wrenCall(vm, class_methods.end_play_handle);
    }

    Entity Component::get_entity() const {
        auto& registry = g_engine->get_registry();

        auto* handle = registry.get<WrenHandle*>(entity);

        return Entity{handle, entity, registry};
    }
} // namespace horus

#pragma region Wren bindings
/*
 * Everything in this region is auto-generated when the code is re-built. You should not put any code you care about in this region, nor
 * should you modify the code in this region in any way
 */

// ReSharper disable once CppInconsistentNaming
void _entity_get_tags(WrenVM* vm) {
    auto* entity = static_cast<horus::Entity*>(wrenGetSlotForeign(vm, 0));

    const auto& tags = entity->get_tags();

    wrenSetSlotNewList(vm, 0);

    uint32_t i{0};
    for(const auto& tag : tags) {
        wrenSetSlotString(vm, 1, tag.c_str());
        wrenInsertInList(vm, 0, i, 1);

        i++;
    }
}

// ReSharper disable once CppInconsistentNaming
void _entity_get_world(WrenVM* vm) {
    auto* entity = static_cast<horus::Entity*>(wrenGetSlotForeign(vm, 0));
    auto* world = entity->get_world();

    if(world) {
        auto* handle = world->_get_wren_handle();
        wrenSetSlotHandle(vm, 0, handle);
    } else {
        wrenAbortFiber(vm, 1);
    }
}

// ReSharper disable once CppInconsistentNaming
void _component_get_entity(WrenVM* vm) {
    auto* component = static_cast<horus::Component*>(wrenGetSlotForeign(vm, 0));

    const auto entity = component->get_entity();

    const auto entity_handle = entity.get_handle();
}

// ReSharper disable once CppInconsistentNaming
void _scripting_entity_scripting_api_register_with_scripting_runtime(horus::ScriptingRuntime& runtime) {
    runtime.register_script_function({.module_name = "sanity_engine",
                                      .class_name = "Entity",
                                      .is_static = false,
                                      .method_signature = "get_tags()"},
                                     _entity_get_tags);

    runtime.register_script_function({.module_name = "sanity_engine",
                                      .class_name = "Entity",
                                      .is_static = false,
                                      .method_signature = "get_world()"},
                                     _entity_get_world);

    runtime.register_script_function({.module_name = "sanity_engine",
                                      .class_name = "Component",
                                      .is_static = false,
                                      .method_signature = "get_entity()"},
                                     _component_get_entity);
}
#pragma endregion
