#pragma once

#include "GLFW/glfw3.h"
#include "core/async/synchronized_resource.hpp"
#include "core/types.hpp"
#include "entt/entity/fwd.hpp"
#include "glm/vec2.hpp"

/*!
 * \brief Simple controller for a simple flycam
 */
class FlycamController {
public:
    explicit FlycamController(GLFWwindow* window_in, entt::entity controlled_entity_in, SynchronizedResource<entt::registry>& registry_in);

    void update_player_transform(Float32 delta_time);

    void set_enabled(bool enabled_in);

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
    SynchronizedResource<entt::registry>* registry_ptr;

    glm::dvec2 last_mouse_pos{};
	
    bool enabled{false};
};
