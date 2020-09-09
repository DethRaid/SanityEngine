#include "first_person_controller.hpp"

#include "GLFW/glfw3.h"
#include "core/components.hpp"
#include "entt/entity/registry.hpp"
#include "glm/ext/quaternion_transform.hpp"
#include "rx/core/log.h"
#include "world/terrain.hpp"

RX_LOG("FirstPersonController", logger);

FirstPersonController::FirstPersonController(GLFWwindow* window_in,
                                             const entt::entity controlled_entity_in,
                                             SynchronizedResource<entt::registry>& registry_in)
    : window{window_in}, controlled_entity{controlled_entity_in}, registry{&registry_in} {
    auto locked_registry = registry->lock();
    // Quick validation
    RX_ASSERT(locked_registry->has<TransformComponent>(controlled_entity), "Controlled entity must have a transform");

    previous_location = locked_registry->get<TransformComponent>(controlled_entity).location;

    glfwGetCursorPos(window, &last_mouse_pos.x, &last_mouse_pos.y);
}

void FirstPersonController::set_current_terrain(Terrain& terrain_in) { terrain = &terrain_in; }

void FirstPersonController::update_player_transform(const Float64 delta_time) {
    // TODO: I'll probably eventually want some kind of momentum, but that can happen later

    auto locked_registry = registry->lock();
    auto& player_transform = locked_registry->get<TransformComponent>(controlled_entity);

    previous_location = player_transform.location;

    const auto forward = player_transform.get_forward_vector();
    const auto right = player_transform.get_right_vector();
    const auto up = player_transform.get_up_vector();

    if(is_grounded) {
        const auto forward_move_vector = normalize(glm::dvec3{forward.x, 0, forward.z});
        const auto right_move_vector = normalize(glm::dvec3{right.x, 0, right.z});

        velocity = glm::vec3{0};

        // Translation
        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            // Move the player entity in its forward direction
            velocity -= forward_move_vector * normal_move_speed;

        } else if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            // Move the player entity in its backward direction
            velocity += forward_move_vector * normal_move_speed;
        }

        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            // Move the player entity in its right direction
            velocity += right_move_vector * normal_move_speed;

        } else if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            // Move the player entity in its left direction
            velocity -= right_move_vector * normal_move_speed;
        }

        if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            velocity.y = jump_velocity;
            is_grounded = false;
        }
    } else {
        // Gravity
        velocity.y -= 9.8 * delta_time;
    }

    player_transform.location += velocity * delta_time;

    // Make sure they're on the terrain
    if(terrain) {
        const auto height = terrain->get_terrain_height(Vec2d{player_transform.location.x, player_transform.location.z});
        if(player_transform.location.y < height + 1.51f) {
            player_transform.location.y = height + 1.5f;

            if(!is_grounded) {
                // If the player has just landed on the ground, reset their vertical velocity
                velocity.y = 0;
            }

            is_grounded = true;

        } else {
            is_grounded = false;
        }
    }

    // Rotation
    glm::dvec2 mouse_pos;
    glfwGetCursorPos(window, &mouse_pos.x, &mouse_pos.y);

    const auto mouse_delta = mouse_pos - last_mouse_pos;

    last_mouse_pos = mouse_pos;

    const auto pitch_delta = std::atan2(mouse_delta.y * 0.0001, 1);
    const auto yaw_delta = std::atan2(mouse_delta.x * 0.0001, 1);

    player_transform.rotation = glm::rotate(player_transform.rotation, yaw_delta, glm::dvec3{0, 1, 0});
    player_transform.rotation = glm::rotate(player_transform.rotation, pitch_delta, right);
}
