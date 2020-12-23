#pragma once

#include <filesystem>

#include "adapters/rex/rex_wrapper.hpp"
#include "core/Prelude.hpp"
#include "core/asset_registry.hpp"
#include "entt/entity/registry.hpp"
#include "input/input_manager.hpp"
#include "player/first_person_controller.hpp"
#include "renderer/renderer.hpp"
#include "rx/console/context.h"
#include "rx/core/ptr.h"
#include "rx/core/time/stop_watch.h"
#include "settings.hpp"
#include "stats/framerate_tracker.hpp"
#include "ui/dear_imgui_adapter.hpp"
#include "world/world.hpp"

namespace sanity::engine {
    /*!
     * \brief Main class for my glorious engine
     */
    class SanityEngine {
    public:
        static std::filesystem::path executable_directory;

        /*!
         * \brief Initializes the engine, including loading static data
         */
        explicit SanityEngine(const std::filesystem::path& executable_directory_in);

        /*!
         * \brief De-initializes the engine
         */
        ~SanityEngine();

        void register_tick_function(Rx::Function<void(Float32)>&& tick_function);

        /*!
         * \brief Executes a single frame, updating game logic and rendering the scene
         */
        void tick();

        [[nodiscard]] entt::entity get_player() const;

        /*!
         * \brief Returns the engine-side registry of entities. This should be used for UI components mostly
         */
        [[nodiscard]] SynchronizedResource<entt::registry>& get_global_registry();

        [[nodiscard]] World* get_world() const;

        [[nodiscard]] GLFWwindow* get_window() const;

        [[nodiscard]] renderer::Renderer& get_renderer() const;

    private:
        rex::Wrapper rex;

        Rx::Ptr<InputManager> input_manager;

        Rx::Ptr<renderer::Renderer> renderer;

        Rx::Ptr<DearImguiAdapter> imgui_adapter;

        Rx::Console::Context console_context;

        FramerateTracker framerate_tracker{1000};

        // Rx::Ptr<BveWrapper> bve;

        GLFWwindow* window;

        Rx::Ptr<World> world;

        SynchronizedResource<entt::registry> global_registry;

        /*!
         * \brief Entity which represents the player
         *
         * SanityEngine is a singleplayer engine, end of story. Makes my life easier and increases my sanity :)
         */
        entt::entity player;

        Rx::Ptr<FirstPersonController> player_controller;

        Rx::Ptr<AssetRegistry> asset_registry;

        Rx::Time::StopWatch frame_timer;

        /*!
         * \brief Number of seconds since the engine started running
         */
        Float32 time_since_application_start = 0;

        Float32 accumulator = 0;

        Uint64 frame_count = 0;

        Rx::Vector<Rx::Function<void(Float32)>> tick_functions;

        void register_cvar_change_listeners();

#pragma region Spawning
        void create_planetary_atmosphere();

        void create_first_person_player();
#pragma endregion

#pragma region Update loop
        void render();
#pragma endregion

#pragma region Diagnostics
        Rx::Optional<entt::entity> frametime_display_entity;

        Rx::Optional<entt::entity> console_window_entity;

        void make_frametime_display();

        void destroy_frametime_display();

        void make_console_window();

        void destroy_console_window();
#pragma endregion
    };

    extern SanityEngine* g_engine;

    void initialize_g_engine(const std::filesystem::path& executable_directory);
} // namespace sanity::engine
