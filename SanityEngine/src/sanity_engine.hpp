#pragma once
#include <memory>

#include <entt/entity/registry.hpp>

#include "input/input_manager.hpp"
#include "renderer/renderer.hpp"
#include "settings.hpp"

/*!
 * \brief Main class for my glorious engine
 */
class SanityEngine {
public:
    /*!
     * \brief Initializes the engine, including loading static data
     */
    explicit SanityEngine(const Settings& settings_in);

    /*!
     * \brief De-initializes the engine, flushing all logs
     */
    ~SanityEngine();

    /*!
     * \brief Runs the main loop of the engine. This method eventually returns, after the user is finished playing their game
     */
    void run();

    [[nodiscard]] entt::entity get_player() const;

private:
    Settings settings;

    std::unique_ptr<InputManager> input_manager;

    std::unique_ptr<renderer::Renderer> renderer;

    GLFWwindow* window;

    entt::registry registry;

    /*!
     * \brief Entity which represents the player
     *
     * SanityEngine is a singleplayer engine, end of story. Makes my life easier and increases my sanity :)
     */
    entt::entity player;

#pragma region Debug
    void create_debug_cube();
#pragma endregion

#pragma region Tick
    /*!
     * \brief Updates the movement of the player entity
     *
     * Eventually I'll want to split this out into something... eventually. For now it can be part of the engine class
     */
    void update_player_movement(double delta_time);

    /*!
     * \brief Ticks the engine, advancing time by the specified amount
     */
    void tick(double delta_time);
#pragma endregion
};
