#pragma once

#include "glm/gtc/quaternion.hpp"
#include "glm/ext/quaternion_float.hpp" // This include MUST be after glm/gtc/quaternion.hpp
#include "glm/vec3.hpp"
#include "rx/core/hints/force_inline.h"

namespace sanity::engine {
    struct Transform {
        glm::vec3 location{0};

        glm::quat rotation{glm::vec3{0, 0, 0}};

        glm::vec3 scale{1};

        [[nodiscard]] RX_HINT_FORCE_INLINE glm::vec3 get_forward_vector() const;

        [[nodiscard]] RX_HINT_FORCE_INLINE glm::vec3 get_right_vector() const;

        [[nodiscard]] RX_HINT_FORCE_INLINE glm::vec3 get_up_vector() const;

        [[nodiscard]] RX_HINT_FORCE_INLINE glm::mat4 to_matrix() const;
    };

    inline glm::vec3 Transform::get_forward_vector() const {
        constexpr auto global_forward = glm::vec3{0, 0, 1};
        return global_forward * rotation;
    }

    inline glm::vec3 Transform::get_right_vector() const {
        constexpr auto global_right = glm::vec3{1, 0, 0};
        return global_right * rotation;
    }

    inline glm::vec3 Transform::get_up_vector() const {
        constexpr auto global_up = glm::vec3{0, 1, 0};
        return global_up * rotation;
    }

    inline glm::mat4 Transform::to_matrix() const {
        auto matrix = translate(glm::mat4{1}, location);
        matrix = matrix * static_cast<glm::mat4>(rotation);
        matrix = glm::scale(matrix, scale);

        return matrix;
    }
} // namespace sanity::engine
