#pragma once

#include <GLFW/glfw3.h>
#include <entt/entity/fwd.hpp>

/*!
 * \brief Simple controller for a simple flycam
 */
class FlycamController {
public:
    explicit FlycamController(GLFWwindow* window_in, entt::entity controlled_entity_in, entt::registry& registry_in);

    void update_player_position(float delta_time) const;

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
};
