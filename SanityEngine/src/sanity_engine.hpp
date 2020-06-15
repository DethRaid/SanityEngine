#pragma once
#include <memory>

#include <assimp/Importer.hpp>
#include <entt/entity/registry.hpp>
#include <ftl/task_scheduler.h>
#include <rx/core/ptr.h>

#include "../first_person_controller.hpp"
#include "adapters/rex/rex_wrapper.hpp"
#include "bve/bve_wrapper.hpp"
#include "input/input_manager.hpp"
#include "renderer/renderer.hpp"
#include "settings.hpp"
#include "stats/framerate_tracker.hpp"
#include "ui/dear_imgui_adapter.hpp"
#include "world/world.hpp"

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

    [[nodiscard]] entt::registry& get_registry();

    [[nodiscard]] World* get_world() const;

private:
    rex::Wrapper rex;

    Settings settings;

    Rx::Ptr<InputManager> input_manager;

    Rx::Ptr<renderer::Renderer> renderer;

    Rx::Ptr<DearImguiAdapter> imgui_adapter;

    FramerateTracker framerate_tracker{1000};

    Rx::Ptr<BveWrapper> bve;

    GLFWwindow* window;

    Rx::Ptr<World> world;

    entt::registry registry;

    /*!
     * \brief Entity which represents the player
     *
     * SanityEngine is a singleplayer engine, end of story. Makes my life easier and increases my sanity :)
     */
    entt::entity player;

    Rx::Ptr<FirstPersonController> player_controller;

    Assimp::Importer importer;

    void initialize_scripting_runtime();

    void register_horus_api() const;

    Rx::Ptr<horus::ScriptingRuntime> scripting_runtime;

#pragma region Spawning
    void create_planetary_atmosphere();

    void make_frametime_display();

    void create_first_person_player();

    void load_bve_train(const Rx::String& filename);

    void load_3d_object(const Rx::String& filename);
#pragma endregion
};
