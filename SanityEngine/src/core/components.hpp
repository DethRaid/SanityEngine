#pragma once

#include "core/transform.hpp"
#include "core/types.hpp"
#include "entt/entity/fwd.hpp"
#include "rx/core/map.h"
#include "rx/core/optional.h"
#include "rx/core/string.h"

namespace sanity::engine {
    /*!
     * \brief Component type for any entity within SanityEngine
     *
     * Entities have a system for sending and receiving events. Other components may subscribe to that system and react to events
     */
    struct __declspec(uuid("{6A611962-D937-4FC8-AF7D-7FFE4CD43749}")) [[sanity::component]] SanityEngineEntity {
        Rx::String name;

        Uint64 id;

        Rx::Map<Rx::String, Int32> tags;

        SanityEngineEntity();

        void add_tag(const Rx::String& tag);

        void add_stacks_of_tag(const Rx::String& tag, Int32 num_stacks);

        void remove_tag(const Rx::String& tag);

        void remove_num_tags(const Rx::String& tag, Uint32 num_stacks);
    };

    struct _declspec(uuid("{DDC37FE8-B703-4132-BD17-0F03369A434A}")) [[sanity::component]] TransformComponent {
        Transform transform;
    };

    struct __declspec(uuid("{BC22F5FC-A56D-481F-843E-49BD98A84ED4}")) [[sanity::component]] HierarchyComponent {
        Rx::Optional<entt::entity> parent{Rx::nullopt};

        Rx::Vector<entt::entity> children;
    };
} // namespace sanity::engine
