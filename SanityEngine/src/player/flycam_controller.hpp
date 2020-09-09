#pragma once

#include "GLFW/glfw3.h"
#include "core/types.hpp"
#include "entt/entity/fwd.hpp"
#include "glm/vec2.hpp"

/*!
 * \brief Simple controller for a simple flycam
 */
class FlycamController {
public:
    explicit FlycamController(GLFWwindow* window_in, entt::entity controlled_entity_in, entt::registry& registry_in);

    void update_player_transform(Float64 delta_time);

private:
    /*!
     * \brief Window that will receive input
     */
    GLFWwindow* window;

    /*!
     * \brief The entity which represents the player
     */
    entt::entity controlled_entity;

    /*!
     * \brief Registry where all the player's components are stored
     */
    entt::registry* registry;

    glm::dvec2 last_mouse_pos;
};
