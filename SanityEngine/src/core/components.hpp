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

    	[[nodiscard]] glm::mat4 get_world_matrix(const entt::registry& registry) const;

    	[[nodiscard]] Transform* operator->();

        [[nodiscard]] const Transform* operator->() const;
    };
    
    void draw_component_editor(TransformComponent& transform);
} // namespace sanity::engine
