#pragma once

#include <unordered_set>

#include <glm/ext/quaternion_float.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct TransformComponent {
    glm::vec3 position{0};

    glm::quat rotation{};

    glm::vec3 scale{1};

    [[nodiscard]] __forceinline glm::vec3 get_forward_vector() const;

    [[nodiscard]] __forceinline glm::vec3 get_right_vector() const;

    [[nodiscard]] __forceinline glm::vec3 get_up_vector() const;
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

struct TagComponent {
    std::unordered_set<std::string> tags;
};
