#include "entity_scripting_api.hpp"

#include "globals.hpp"
#include "rx/core/string.h"
#include "sanity_engine.hpp"
#include "scripting_runtime.hpp"
#include "world/world.hpp"
#include "wren.hpp"

namespace script {
    Entity::Entity(WrenHandle* handle_in, const entt::entity entity_in, SynchronizedResource<entt::registry>& registry_in)
        : handle{handle_in}, entity{entity_in}, registry{&registry_in} {}

    void Entity::add_tag(const Rx::String& tag) const {
        auto locked_registry = registry->lock();
        auto& tags = locked_registry->get_or_assign<SanityEngineEntity>(entity);
        if(auto* num_stacks = tags.tags.find(tag)) {
            (*num_stacks)++;
        } else {
            tags.tags.insert(tag, 1);
        }
    }

    bool Entity::has_tag(const Rx::String& tag) const {
        auto locked_registry = registry->lock();
        if(locked_registry->has<SanityEngineEntity>(entity)) {
            const auto& tags = locked_registry->get<SanityEngineEntity>(entity);
            return tags.tags.find(tag) != nullptr;
        }

        return false;
    }

    Rx::Map<Rx::String, Int32> Entity::get_tags() const {
        auto locked_registry = registry->lock();
        if(locked_registry->has<SanityEngineEntity>(entity)) {
            const auto& tag_component = locked_registry->get<SanityEngineEntity>(entity);
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

    void Component::tick(const Float32 delta_seconds) const {
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

        auto* handle = registry.lock()->get<WrenHandle*>(entity);

        return Entity{handle, entity, registry};
    }
} // namespace script

#pragma region Wren bindings
/*
 * Everything in this region is auto-generated when the code is re-built. You should not put any code you care about in this region, nor
 * should you modify the code in this region in any way
 */

// ReSharper disable once CppInconsistentNaming
void _entity_get_tags(WrenVM* vm) {
    auto* entity = static_cast<script::Entity*>(wrenGetSlotForeign(vm, 0));

    const auto& tags = entity->get_tags();

    wrenSetSlotNewList(vm, 0);

    Uint32 i{0};
    tags.each_key([&](const Rx::String& tag) {
        wrenSetSlotString(vm, 1, tag.data());
        wrenInsertInList(vm, 0, i, 1);

        i++;
    });
}

// ReSharper disable once CppInconsistentNaming
void _entity_get_world(WrenVM* vm) {
    auto* entity = static_cast<script::Entity*>(wrenGetSlotForeign(vm, 0));
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
    auto* component = static_cast<script::Component*>(wrenGetSlotForeign(vm, 0));

    const auto entity = component->get_entity();

    const auto entity_handle = entity.get_handle();
}

// ReSharper disable once CppInconsistentNaming
void _scripting_entity_scripting_api_register_with_scripting_runtime(script::ScriptingRuntime& runtime) {
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

    runtime.register_script_object_allocator(
        script::ScriptingClassName{.module_name = "sanity_engine", .class_name = "Entity"},
        WrenForeignClassMethods{.allocate = +[](WrenVM* vm) { auto* data = wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenHandle*)); },
                                .finalize = +[](void* data) {}});
}
#pragma endregion
