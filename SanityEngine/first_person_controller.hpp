#pragma once

#include <memory>

#include <entt/entity/fwd.hpp>
#include <glm/vec2.hpp>
#include <spdlog/logger.h>

class Terrain;

struct GLFWwindow;

class FirstPersonController {
public:
    explicit FirstPersonController(GLFWwindow* window_in, entt::entity controlled_entity_in, entt::registry& registry_in);

    void update_player_transform(float delta_time);

    void set_current_terrain(Terrain& terrain_in);

private:
    static std::shared_ptr<spdlog::logger> logger;
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

    Terrain* terrain{nullptr};
};
