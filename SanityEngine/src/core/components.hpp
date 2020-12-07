#pragma once

#include "core/types.hpp"
#include "entt/entity/entity.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/ext/quaternion_float.hpp" // This include MUST be after glm/gtc/quaternion.hpp
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
        glm::vec3 location{0};

        glm::quat rotation{};

        glm::vec3 scale{1};

        [[nodiscard]] RX_HINT_FORCE_INLINE glm::vec3 get_forward_vector() const;

        [[nodiscard]] RX_HINT_FORCE_INLINE glm::vec3 get_right_vector() const;

        [[nodiscard]] RX_HINT_FORCE_INLINE glm::vec3 get_up_vector() const;

        [[nodiscard]] RX_HINT_FORCE_INLINE glm::mat4 to_matrix() const;
    };

    struct __declspec(uuid("{BC22F5FC-A56D-481F-843E-49BD98A84ED4}")) [[sanity::component]] HierarchyComponent {
        Rx::Optional<entt::entity> parent{Rx::nullopt};

        Rx::Vector<entt::entity> children;
    };

    inline glm::vec3 TransformComponent::get_forward_vector() const {
        constexpr auto global_forward = glm::vec3{0, 0, 1};
        return global_forward * rotation;
    }

    inline glm::vec3 TransformComponent::get_right_vector() const {
        constexpr auto global_right = glm::vec3{1, 0, 0};
        return global_right * rotation;
    }

    inline glm::vec3 TransformComponent::get_up_vector() const {
        constexpr auto global_up = glm::vec3{0, 1, 0};
        return global_up * rotation;
    }

    inline glm::mat4 TransformComponent::to_matrix() const {
        auto matrix = translate({}, location);
        matrix = matrix * static_cast<glm::mat4>(rotation);
        matrix = glm::scale(matrix, scale);

        return matrix;
    }
} // namespace sanity::engine
