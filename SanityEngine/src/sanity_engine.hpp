#pragma once
#include <memory>

#include <assimp/Importer.hpp>
#include <entt/entity/registry.hpp>
#include <ftl/task_scheduler.h>

#include "../first_person_controller.hpp"
#include "bve/bve_wrapper.hpp"
#include "input/input_manager.hpp"
#include "renderer/renderer.hpp"
#include "settings.hpp"
#include "stats/framerate_tracker.hpp"
#include "ui/dear_imgui_adapter.hpp"
#include "world/world.hpp"

inline std::unique_ptr<ftl::TaskScheduler> task_scheduler;

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
    std::shared_ptr<spdlog::logger> logger;

    Settings settings;

    std::unique_ptr<InputManager> input_manager;

    std::unique_ptr<renderer::Renderer> renderer;

    std::unique_ptr<DearImguiAdapter> imgui_adapter;

    FramerateTracker framerate_tracker{1000};

    std::unique_ptr<BveWrapper> bve;

    GLFWwindow* window;

    std::unique_ptr<World> world;

    entt::registry registry;

    /*!
     * \brief Entity which represents the player
     *
     * SanityEngine is a singleplayer engine, end of story. Makes my life easier and increases my sanity :)
     */
    entt::entity player;

    std::unique_ptr<FirstPersonController> player_controller;

    Assimp::Importer importer;

    void initialize_scripting_runtime();

#pragma region Debug
    void create_debug_plane();
#pragma endregion

    std::unique_ptr<ScriptingRuntime> scripting_runtime;

#pragma region Spawning
    void create_planetary_atmosphere();

    void make_frametime_display();

    void create_flycam_player();

    void load_bve_train(const std::string& filename);

    void load_3d_object(const std::string& filename);
#pragma endregion
};
