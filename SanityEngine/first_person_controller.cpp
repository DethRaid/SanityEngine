#include "first_person_controller.hpp"

#include <GLFW/glfw3.h>
#include <entt/entity/registry.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "src/core/components.hpp"
#include "src/core/ensure.hpp"
#include "src/world/terrain.hpp"

std::shared_ptr<spdlog::logger> FirstPersonController::logger{spdlog::stdout_color_st("FirstPersonController")};

FirstPersonController::FirstPersonController(GLFWwindow* window_in, const entt::entity controlled_entity_in, entt::registry& registry_in)
    : window{window_in}, controlled_entity{controlled_entity_in}, registry{&registry_in} {
    // Quick validation
    ENSURE(registry->has<TransformComponent>(controlled_entity), "Controlled entity must have a transform");

    logger->set_level(spdlog::level::debug);

    glfwGetCursorPos(window, &last_mouse_pos.x, &last_mouse_pos.y);
}

void FirstPersonController::set_current_terrain(Terrain& terrain_in) { terrain = &terrain_in; }

void FirstPersonController::update_player_transform(const float delta_time) {
    // TODO: I'll probably eventually want some kind of momentum, but that can happen later

    auto& player_transform = registry->get<TransformComponent>(controlled_entity);

    const auto forward = player_transform.get_forward_vector();
    const auto right = player_transform.get_right_vector();
    const auto up = player_transform.get_up_vector();

    // Translation
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        // Move the player entity in its forward direction
        player_transform.position -= forward * delta_time;

    } else if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        // Move the player entity in its backward direction
        player_transform.position += forward * delta_time;
    }

    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        // Move the player entity in its right direction
        player_transform.position += right * delta_time;

    } else if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        // Move the player entity in its left direction
        player_transform.position -= right * delta_time;
    }

    if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        // Move the player entity along the global up direction
        player_transform.position += glm::vec3{0, delta_time, 0};

    } else if(glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        // Move the player along the global down direction
        player_transform.position -= glm::vec3{0, delta_time, 0};
    }

    // Make sure they're on the terrain
    if(terrain) {
        const auto height = terrain->get_terrain_height(glm::vec2{player_transform.position.x, player_transform.position.z});
        if(player_transform.position.y < height + 1.5f) {
            player_transform.position.y = height + 1.5f;
        }
    }

    // Rotation
    glm::dvec2 mouse_pos;
    glfwGetCursorPos(window, &mouse_pos.x, &mouse_pos.y);

    const auto mouse_delta = mouse_pos - last_mouse_pos;

    last_mouse_pos = mouse_pos;

    const auto pitch_delta = std::atan2(mouse_delta.y * 0.0001, 1);
    const auto yaw_delta = std::atan2(mouse_delta.x * 0.0001, 1);

    player_transform.rotation = glm::rotate(player_transform.rotation, static_cast<float>(yaw_delta), glm::vec3{0, 1, 0});
    player_transform.rotation = glm::rotate(player_transform.rotation, static_cast<float>(pitch_delta), right);
}
