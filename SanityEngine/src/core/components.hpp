#pragma once

#include "core/transform.hpp"
#include "entt/entity/fwd.hpp"
#include "rx/core/optional.h"
#include "rx/core/string.h"

namespace sanity::engine {
    struct _declspec(uuid("{DDC37FE8-B703-4132-BD17-0F03369A434A}")) TransformComponent {
        Transform transform;
    	
        Rx::Optional<entt::entity> parent{Rx::nullopt};

        Rx::Vector<entt::entity> children;

    	/**
    	 * @brief Returns the matrix from this node's local transform frame to it's immediate parent's transform frame
       	 */
        [[nodiscard]] glm::mat4 get_local_matrix() const;

    	/**
    	 * @brief Returns the matrix that transforms from this node's local transform frame to the world transform frame
    	 */
    	[[nodiscard]] glm::mat4 get_model_matrix(const entt::registry& registry) const;

    	[[nodiscard]] Transform* operator->();

        [[nodiscard]] const Transform* operator->() const;
    };
    
    void draw_component_properties(TransformComponent& transform);
} // namespace sanity::engine
