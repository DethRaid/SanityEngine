#pragma once

#include "core/types.hpp"
#include "glm/ext/quaternion_float.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "rx/core/map.h"
#include "rx/core/string.h"
#include "serialization/serialization.hpp"

// The horus::component generates a GUID handle for the Horus scripting system to use when creating a component

struct TransformComponent {
    glm::dvec3 location{0};

    glm::dquat rotation{};

    glm::dvec3 scale{1};

    [[nodiscard]] __forceinline glm::dvec3 get_forward_vector() const;

    [[nodiscard]] __forceinline glm::dvec3 get_right_vector() const;

    [[nodiscard]] __forceinline glm::dvec3 get_up_vector() const;

    [[nodiscard]] __forceinline glm::dmat4 to_matrix() const;

#pragma region Autogenerated

    // Autogenerated identifier for this component type. Allows this component to type to be saved to disk and
    // ReSharper disable once CppInconsistentNaming
    uint64_t _handle{0};
#pragma endregion
};

inline glm::dvec3 TransformComponent::get_forward_vector() const {
    constexpr auto global_forward = glm::dvec3{0, 0, 1};
    return global_forward * rotation;
}

inline glm::dvec3 TransformComponent::get_right_vector() const {
    constexpr auto global_right = glm::dvec3{1, 0, 0};
    return global_right * rotation;
}

inline glm::dvec3 TransformComponent::get_up_vector() const {
    constexpr auto global_up = glm::dvec3{0, 1, 0};
    return global_up * rotation;
}

inline glm::dmat4 TransformComponent::to_matrix() const {
    auto matrix = translate({}, location);
    matrix = matrix * static_cast<glm::dmat4>(rotation);
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

JSON5_CLASS(TransformComponent, location, rotation, scale)
