#include "flycam_controller.hpp"

#include <entt/entity/registry.hpp>
#include <minitrace.h>

#include "../core/components.hpp"
#include "../core/ensure.hpp"

FlycamController::FlycamController(GLFWwindow* window_in, const entt::entity controlled_entity_in, entt::registry& registry_in)
    : window{window_in}, controlled_entity{controlled_entity_in}, registry{&registry_in} {
    // Quick validation
    ENSURE(registry->has<TransformComponent>(controlled_entity), "Controlled entity must have a transform");
}

void FlycamController::update_player_position(const float delta_time) const {
    MTR_SCOPE("FlycamController", "update_player_position");

    auto& player_transform = registry->get<TransformComponent>(controlled_entity);
    const auto forward = player_transform.get_forward_vector();

    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        // Move the player entity in its forward direction
        player_transform.position += forward * delta_time;

    } else if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        // Move the player entity in its backward direction
        player_transform.position -= forward * delta_time;
    }

    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        // Move the player entity in its right direction

    } else if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        // Move the player entity in its left direction
    }

    if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        // Move the player entity along the global up direction
        player_transform.position += glm::vec3{0, 0, 1} * delta_time;

    } else if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        // Move the player along the global down direction
        player_transform.position -= glm::vec3{0, 0, 1} * delta_time;
    }
}
