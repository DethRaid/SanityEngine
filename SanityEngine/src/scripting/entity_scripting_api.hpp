#pragma once

#include <concepts>

#include <entt/entity/fwd.hpp>
#include <entt/entity/registry.hpp>
#include <rx/core/map.h>
#include <rx/core/set.h>
#include <rx/core/string.h>
#include <wren/wren.hpp>

#include "core/types.hpp"
#include "core/async/synchronized_resource.hpp"

namespace Rx {
    struct String;
}

class World;

namespace horus {
    class ScriptingRuntime;

    struct ScriptComponentMethods {
        WrenHandle* init_handle;

        WrenHandle* begin_play_handle;

        WrenHandle* tick_handle;

        WrenHandle* end_play_handle;
    };

    enum class LifetimeStage {
        /*!
         * \brief The C++ representation of the component has been created and fully initialized, but the Wren representation has not
         */
        DefaultObject,

        /*!
         * \brief The Wren object has been initialized and the Wren component it ready for the game world
         */
        ReadyToTick
    };

    template <typename T>
    concept NativeComponent = requires(T a) {
        { a._horus_handle }
        ->std::convertible_to<WrenHandle*>;
    };

    class [[horus::class(module = sanity_engine)]] Entity {
    public:
        [[horus::constructor]] explicit Entity(WrenHandle * handle_in,
                                               entt::entity entity_in,
                                               SynchronizedResource<entt::registry>& registry_in);

        [[horus::method]] void add_tag(const Rx::String& tag) const;

        [[horus::method]] [[nodiscard]] bool has_tag(const Rx::String& tag) const;

        [[horus::method]] [[nodiscard]] Rx::Map<Rx::String, Int32> get_tags() const;

        [[horus::method]] [[nodiscard]] World* get_world() const;

        /*!
         * \brief Retrieves a component of the given type
         *
         * This one is going to need a lof of autogen hackery to make it work. I'm thinking that I'll generate a GUID for each component
         * type, then autogen Wren constants for those component. The binding function that I autogen for this method will have to map
         * from those GUIDs to C++ types, which will generate calls into this method
         */
        template <NativeComponent ComponentType>
        [[horus::method]] ComponentType& get_component() const;

        [[nodiscard]] WrenHandle* get_handle() const;

    private:
        WrenHandle* handle;

        entt::entity entity;

        SynchronizedResource<entt::registry>* registry;
    };

    template <NativeComponent ComponentType>
    ComponentType& Entity::get_component() const {
        return registry.get<ComponentType>(entity);
    }

    class [[horus::class(module = sanity_engine)]] Component {
    public:
        LifetimeStage lifetime_stage{LifetimeStage::DefaultObject};

        explicit Component(entt::entity entity_in, WrenHandle * handle_in, const ScriptComponentMethods& class_methods_in, WrenVM& vm_in);

        Component(const Component& other) = default;
        Component& operator=(const Component& other) = default;

        Component(Component && old) noexcept = default;
        Component& operator=(Component&& old) noexcept = default;

        ~Component() = default;

        [[horus::implemented_in_script]] void initialize_self();

        [[horus::implemented_in_script]] void begin_play(World & world) const;

        [[horus::implemented_in_script]] void tick(Float32 delta_seconds) const;

        [[horus::implemented_in_script]] void end_play() const;

        [[horus::method]] [[nodiscard]] Entity get_entity() const;

    private:
        entt::entity entity;

        ScriptComponentMethods class_methods;

        WrenHandle* component_handle;

        WrenVM* vm;
    };
} // namespace horus

#pragma region Wren bindings
/*
 * Everything in this region is auto-generated when the code is re-built. You should not put any code you care about in this region, nor
 * should you modify the code in this region in any way
 */

// ReSharper disable once CppInconsistentNaming
void _scripting_entity_scripting_api_register_with_scripting_runtime(horus::ScriptingRuntime& runtime);
#pragma endregion
