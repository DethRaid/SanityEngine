#include "core/components.hpp"

#include "entt/entity/registry.hpp"
#include "ui/property_drawers.hpp"

namespace sanity::engine {
    glm::mat4 TransformComponent::get_world_matrix(const entt::registry& registry) const {
        const auto local_matrix = transform.to_matrix();

        if(parent) {
            const auto& parent_transform = registry.get<TransformComponent>(*parent);
            return local_matrix * parent_transform.get_world_matrix(registry);

        } else {
            return local_matrix;
        }
    }

    void draw_component_editor(TransformComponent& transform) { ui::draw_property_editor("transform", transform.transform); }
} // namespace sanity::engine
