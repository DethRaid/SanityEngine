#pragma once

#include "core/types.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/ext/quaternion_float.hpp"
#include "rx/core/map.h"
#include "rx/core/string.h"

struct _declspec(uuid("DDC37FE8-B703-4132-BD17-0F03369A434A")) [[sanity::component]] TransformComponent {
    glm::vec3 location{0};

    glm::quat rotation{};

    glm::vec3 scale{1};

    [[nodiscard]] RX_HINT_FORCE_INLINE glm::vec3 get_forward_vector() const;

    [[nodiscard]] RX_HINT_FORCE_INLINE glm::vec3 get_right_vector() const;

    [[nodiscard]] RX_HINT_FORCE_INLINE glm::vec3 get_up_vector() const;

    [[nodiscard]] RX_HINT_FORCE_INLINE glm::mat4 to_matrix() const;
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

/*!
 * \brief Component type for any entity within SanityEngine
 *
 * Entities have a system for sending and receiving events. Other components may subscribe to that system and react to events
 */
struct SanityEngineEntity {
    Rx::Map<Rx::String, Int32> tags;

    void add_tag(const Rx::String& tag);

    void add_stacks_of_tag(const Rx::String& tag, Int32 num_stacks);

    void remove_tag(const Rx::String& tag, bool remove_all_stacks = false);
};
