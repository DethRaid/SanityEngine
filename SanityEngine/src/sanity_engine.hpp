#pragma once

#include <filesystem>

#include "adapters/rex/rex_wrapper.hpp"
#include "core/Prelude.hpp"
#include "core/asset_registry.hpp"
#include "core/reflection/type_reflection.hpp"
#include "entt/entity/registry.hpp"
#include "input/input_manager.hpp"
#include "player/first_person_controller.hpp"
#include "renderer/renderer.hpp"
#include "rx/console/context.h"
#include "rx/core/ptr.h"
#include "rx/core/time/stop_watch.h"
#include "settings.hpp"
#include "stats/framerate_tracker.hpp"
#include "system/system.hpp"
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

        void register_system(const std::string& name, std::unique_ptr<System>&& system);

        /*!
         * \brief Executes a single frame, updating game logic and rendering the scene
         */
        void tick();

        [[nodiscard]] TypeReflection& get_type_reflector();

        [[nodiscard]] entt::entity get_player() const;

        [[nodiscard]] World& get_world();

        /*!
         * \brief Returns the engine-side registry of entities. This should be used for UI components mostly
         */
        [[nodiscard]] entt::registry& get_entity_registry();

        [[nodiscard]] GLFWwindow* get_window() const;

        [[nodiscard]] renderer::Renderer& get_renderer() const;

        [[nodiscard]] InputManager& get_input_manager() const;
    	
        [[nodiscard]] Uint32 get_frame_count() const;

    private:
        rex::Wrapper rex;

        TypeReflection type_reflector;

        Rx::Ptr<InputManager> input_manager;

        Rx::Ptr<renderer::Renderer> renderer;

        Rx::Ptr<DearImguiAdapter> imgui_adapter;

        Rx::Console::Context console_context;

        FramerateTracker framerate_tracker{1000};

        // Rx::Ptr<BveWrapper> bve;

        GLFWwindow* window;

        entt::registry global_registry;

        World world;

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

        void register_engine_component_type_reflection();

#pragma region Spawning
        void create_first_person_player();
#pragma endregion

#pragma region Update loop
        std::unordered_map<std::string, std::unique_ptr<System>> systems;

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
