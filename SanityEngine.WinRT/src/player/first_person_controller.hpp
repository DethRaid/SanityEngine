#pragma once

#include "core/async/synchronized_resource.hpp"
#include "entt/entity/fwd.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

class Terrain;

struct GLFWwindow;

class FirstPersonController {
public:
    explicit FirstPersonController(GLFWwindow* window_in,
                                   entt::entity controlled_entity_in,
                                   SynchronizedResource<entt::registry>& registry_in);

    void update_player_transform(float delta_time);

    void set_current_terrain(Terrain& terrain_in);

private:
    float normal_move_speed = 5;

    float jump_velocity = 5;

    /*!
     * \brief Window that will receive input
     *
     * This player controller queries GLFW for key states directly. This may or may not be a Bad Idea
     */
    GLFWwindow* window;

    /*!
     * \brief The entity which represents the player
     */
    entt::entity controlled_entity;

    /*!
     * \brief Registry where all the player's components are stored
     */
    SynchronizedResource<entt::registry>* registry;

    glm::dvec2 last_mouse_pos;

    glm::vec3 previous_location{};
    glm::vec3 velocity{0};

    Terrain* terrain{nullptr};
    bool is_grounded{true};
};
