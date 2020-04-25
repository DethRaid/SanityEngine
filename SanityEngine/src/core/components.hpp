#pragma once

#include <glm/glm.hpp>

struct TransformComponent {
    glm::vec3 position;

    glm::vec3 rotation;

    glm::vec3 scale;

    [[nodiscard]] __forceinline glm::vec3 get_forward_vector() const;
};

inline glm::vec3 TransformComponent::get_forward_vector() const {
    const auto cos_pitch = cos(rotation.x);

    return {cos_pitch * sin(rotation.z), cos_pitch * cos(rotation.z), sin(rotation.z)};
}
