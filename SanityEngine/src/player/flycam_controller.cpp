#include "flycam_controller.hpp"

#include "core/components.hpp"
#include "entt/entity/registry.hpp"
#include "glm/ext/quaternion_transform.hpp"
#include "rx/core/log.h"

RX_LOG("FlycamController", logger);

constexpr Float32 X_SENSITIVITY = 0.005f;
constexpr Float32 Y_SENSITIVITY = 0.005f;

FlycamController::FlycamController(GLFWwindow* window_in,
                                   const entt::entity controlled_entity_in,
                                   SynchronizedResource<entt::registry>& registry_in)
    : window{window_in}, controlled_entity{controlled_entity_in}, registry_ptr{&registry_in} {
    // Quick validation
    auto registry = registry_ptr->lock();
    RX_ASSERT(registry->has<sanity::engine::TransformComponent>(controlled_entity), "Controlled entity must have a transform");

    glfwGetCursorPos(window, &last_mouse_pos.x, &last_mouse_pos.y);
}

void FlycamController::update_player_transform(const Float32 delta_time) {
    // TODO: I'll probably eventually want some kind of momentum, but that can happen later

    auto registry = registry_ptr->lock();

    auto& player_transform_component = registry->get<sanity::engine::TransformComponent>(controlled_entity);
    auto& player_transform = player_transform_component.transform;

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

    player_transform.rotation = rotate(player_transform.rotation, static_cast<Float32>(mouse_delta.y * X_SENSITIVITY), right);
    player_transform.rotation = rotate(player_transform.rotation, static_cast<Float32>(mouse_delta.x * Y_SENSITIVITY), up);
}
