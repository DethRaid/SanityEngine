#include "flycam_controller.hpp"

#include "core/components.hpp"
#include "entt/entity/registry.hpp"
#include "glm/ext/quaternion_transform.hpp"
#include "rx/core/log.h"

RX_LOG("FlycamController", logger);

FlycamController::FlycamController(GLFWwindow* window_in, const entt::entity controlled_entity_in, entt::registry& registry_in)
    : window{window_in}, controlled_entity{controlled_entity_in}, registry{&registry_in} {
    // Quick validation
    RX_ASSERT(registry->has<TransformComponent>(controlled_entity), "Controlled entity must have a transform");

    glfwGetCursorPos(window, &last_mouse_pos.x, &last_mouse_pos.y);
}

void FlycamController::update_player_transform(const Float32 delta_time) {
    // TODO: I'll probably eventually want some kind of momentum, but that can happen later

    auto& player_transform = registry->get<TransformComponent>(controlled_entity);

    const auto forward = player_transform.get_forward_vector();
    const auto right = player_transform.get_right_vector();
    const auto up = player_transform.get_up_vector();

    // Translation
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        // Move the player entity in its forward direction
        player_transform.location -= forward * delta_time;

    } else if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        // Move the player entity in its backward direction
        player_transform.location += forward * delta_time;
    }

    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        // Move the player entity in its right direction
        player_transform.location += right * delta_time;

    } else if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        // Move the player entity in its left direction
        player_transform.location -= right * delta_time;
    }

    if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        // Move the player entity along the global up direction
        player_transform.location += glm::vec3{0, delta_time, 0};

    } else if(glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        // Move the player along the global down direction
        player_transform.location -= glm::vec3{0, delta_time, 0};
    }

    // Rotation
    glm::dvec2 mouse_pos;
    glfwGetCursorPos(window, &mouse_pos.x, &mouse_pos.y);

    const auto mouse_delta = mouse_pos - last_mouse_pos;

    last_mouse_pos = mouse_pos;

    const auto pitch_delta = Rx::Math::atan2(mouse_delta.y * 0.0001, 1);
    const auto yaw_delta = Rx::Math::atan2(mouse_delta.x * 0.0001, 1);

    player_transform.rotation = rotate(player_transform.rotation, pitch_delta, right);
    player_transform.rotation = rotate(player_transform.rotation, yaw_delta, up);
}
