#include "first_person_controller.hpp"

#include "core/components.hpp"
#include "entt/entity/registry.hpp"
#include "glm/ext/quaternion_transform.hpp"
#include "input\PlatformInput.hpp"
#include "rx/core/log.h"
#include "world/terrain.hpp"

RX_LOG("FirstPersonController", logger);

FirstPersonController::FirstPersonController(const PlatformInput& input_in,
                                             const entt::entity controlled_entity_in,
                                             SynchronizedResource<entt::registry>& registry_in)
    : input{input_in}, controlled_entity{controlled_entity_in}, registry{&registry_in} {
    auto locked_registry = registry->lock();
    // Quick validation
    RX_ASSERT(locked_registry->has<TransformComponent>(controlled_entity), "Controlled entity must have a transform");

    previous_location = locked_registry->get<TransformComponent>(controlled_entity).location;

    last_cursor_location = input.get_mouse_location();
}

void FirstPersonController::set_current_terrain(Terrain& terrain_in) { terrain = &terrain_in; }

void FirstPersonController::update_player_transform(const Float32 delta_time) {
    // TODO: I'll probably eventually want some kind of momentum, but that can happen later

    auto locked_registry = registry->lock();
    auto& player_transform = locked_registry->get<TransformComponent>(controlled_entity);

    previous_location = player_transform.location;

    const auto forward = player_transform.get_forward_vector();
    const auto right = player_transform.get_right_vector();
    const auto up = player_transform.get_up_vector();

    if(is_grounded) {
        const auto forward_move_vector = normalize(glm::vec3{forward.x, 0, forward.z});
        const auto right_move_vector = normalize(glm::vec3{right.x, 0, right.z});

        velocity = glm::vec3{0};

        // Translation
        if(input.is_key_down(InputKey::W)) {
            // Move the player entity in its forward direction
            velocity -= forward_move_vector * normal_move_speed;

        } else if(input.is_key_down(InputKey::S)) {
            // Move the player entity in its backward direction
            velocity += forward_move_vector * normal_move_speed;
        }

        if(input.is_key_down(InputKey::D)) {
            // Move the player entity in its right direction
            velocity += right_move_vector * normal_move_speed;

        } else if(input.is_key_down(InputKey::A)) {
            // Move the player entity in its left direction
            velocity -= right_move_vector * normal_move_speed;
        }

        if(input.is_key_down(InputKey::SPACE)) {
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
        const auto height = terrain->get_terrain_height(Double2{player_transform.location.x, player_transform.location.z});
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
    const auto cursor_location = input.get_mouse_location();

    const auto mouse_delta = cursor_location - last_cursor_location;

    last_cursor_location = cursor_location;

    const auto pitch_delta = Rx::Math::atan2(mouse_delta.y * 0.0001, 1);
    const auto yaw_delta = Rx::Math::atan2(mouse_delta.x * 0.0001, 1);

    player_transform.rotation = rotate(player_transform.rotation, yaw_delta, glm::vec3{0, 1, 0});
    player_transform.rotation = rotate(player_transform.rotation, pitch_delta, right);
}
