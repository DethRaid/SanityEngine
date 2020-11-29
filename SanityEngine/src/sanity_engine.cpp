﻿#include "sanity_engine.hpp"

#define STB_IMAGE_IMPLEMENTATION

#include <filesystem>

#include "GLFW/glfw3.h"
#include "TracyD3D12.hpp"
#include "adapters/rex/rex_wrapper.hpp"
#include "adapters/tracy.hpp"
#include "glm/ext/quaternion_trigonometric.hpp"
#include "loading/entity_loading.hpp"
#include "renderer/rhi/render_device.hpp"
#include "rx/core/abort.h"
#include "rx/core/log.h"
#include "stb_image.h"
#include "ui/ConsoleWindow.hpp"
#include "ui/fps_display.hpp"
#include "ui/ui_components.hpp"
#include "world/generation/gpu_terrain_generation.hpp"
#include "world/world.hpp"

namespace sanity::engine {
    static Rx::GlobalGroup s_sanity_engine_globals{"SanityEngine"};

    RX_LOG("SanityEngine", logger);

    RX_CONSOLE_FVAR(simulation_timestep, "Timestep", "Timestep of SanityEngine's simulation, in seconds", 0.0f, 1.0f, 0.0069f);

    RX_CONSOLE_BVAR(show_frametime_display,
                    "Debug.ShowFramerateWindow",
                    "Show the Dear ImGUI window that displays SanityEngine's render framerate",
                    false);

    RX_CONSOLE_BVAR(show_console, "ShowConsole", "Show the SanityEngine command console", true);
    RX_CONSOLE_SVAR(cvar_ini_file_name, "Console.IniFileName", "Filename of the file to read console variables from", "cvars.ini");

    SanityEngine* g_engine{nullptr};

    struct AtmosphereMaterial {
        glm::vec3 sun_vector;
    };

    static void error_callback(const int error, const char* description) { logger->error("%s (GLFW error %d}", description, error); }

    static void key_func(GLFWwindow* window, const int key, int /* scancode */, const int action, const int mods) {
        auto* input_manager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

        input_manager->on_key(key, action, mods);
    }

    const char* SanityEngine::executable_directory;

    SanityEngine::SanityEngine(const char* executable_directory_in) : input_manager{Rx::make_ptr<InputManager>(RX_SYSTEM_ALLOCATOR)} {
        logger->info("HELLO HUMAN");

        executable_directory = executable_directory_in;

        const auto cvar_ini_filepath = Rx::String::format("%s/%s", executable_directory, *cvar_ini_file_name);
        const auto loaded_cvars = console_context.load(cvar_ini_filepath.data());
        if(!loaded_cvars) {
            Rx::abort("Could not load cvars from file %s (full path %s)", *cvar_ini_file_name, cvar_ini_filepath);
        }

        {
            ZoneScoped;

            {
                ZoneScopedN("glfwInit");
                if(!glfwInit()) {
                    Rx::abort("Could not initialize GLFW");
                }
            }

            glfwSetErrorCallback(error_callback);

            {
                ZoneScopedN("glfwCreateWindow");
                glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
                window = glfwCreateWindow(1000, 480, "Sanity Engine", nullptr, nullptr);
                if(window == nullptr) {
                    Rx::abort("Could not create GLFW window");
                }
            }

            logger->info("Created window");

            glfwSetWindowUserPointer(window, input_manager.get());

            // TODO: Only enable this in play-in-editor mode
            // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            glfwSetKeyCallback(window, key_func);

            renderer = Rx::make_ptr<renderer::Renderer>(RX_SYSTEM_ALLOCATOR, window);
            logger->info("Initialized renderer");

            asset_registry = Rx::make_ptr<AssetRegistry>(RX_SYSTEM_ALLOCATOR, "data/Content");

            create_first_person_player();

            create_planetary_atmosphere();

            if(*show_frametime_display) {
                make_frametime_display();
            }

            if(*show_console) {
                make_console_window();
            }

            register_cvar_change_listeners();

            imgui_adapter = Rx::make_ptr<DearImguiAdapter>(RX_SYSTEM_ALLOCATOR, window, *renderer);

            terraingen::initialize(renderer->get_render_backend());

            world = World::create({.seed = 666,
                                   .height = 128,
                                   .width = 128,
                                   .max_ocean_depth = 8,
                                   .min_terrain_depth_under_ocean = 8,
                                   .max_height_above_sea_level = 16},
                                  player,
                                  registry,
                                  *renderer);

            if(player_controller) {
                player_controller->set_current_terrain(world->get_terrain());
            }

            create_environment_object_editor();

            frame_timer.start();
        }
    }

    SanityEngine::~SanityEngine() {
        const auto cvar_ini_filepath = Rx::String::format("%s/%s", executable_directory, *cvar_ini_file_name);
        const auto saved_cvars = console_context.save(cvar_ini_filepath.data());
        if(!saved_cvars) {
            Rx::abort("Could not save cvars to file %s (full path %s)", *cvar_ini_file_name, cvar_ini_filepath);
        }

        glfwDestroyWindow(window);

        glfwTerminate();

        logger->warning("REMAIN INDOORS");
    }

    void SanityEngine::do_frame() {
        frame_timer.stop();
        const auto frame_duration = frame_timer.elapsed();
        frame_timer.start();

        const auto frame_duration_seconds = static_cast<Float32>(frame_duration.total_seconds());

        accumulator += frame_duration_seconds;

        const auto delta_time = simulation_timestep->get();

        frame_count++;
        renderer->begin_frame(frame_count);

        while(accumulator >= delta_time) {
            if(player_controller) {
                player_controller->update_player_transform(delta_time);
            }

            if(world) {
                world->tick(delta_time);
            }

            accumulator -= delta_time;
            time_since_application_start += delta_time;
        }

        // TODO: The final touch from https://gafferongames.com/post/fix_your_timestep/

        if(glfwGetWindowAttrib(window, GLFW_VISIBLE) == GLFW_TRUE) {
            // Only render when the window is visible
            render();
        }

        framerate_tracker.add_frame_time(frame_duration_seconds);
    }

    entt::entity SanityEngine::get_player() const { return player; }

    SynchronizedResource<entt::registry>& SanityEngine::get_registry() { return registry; }

    World* SanityEngine::get_world() const { return world.get(); }

    GLFWwindow* SanityEngine::get_window() const { return window; }

    void SanityEngine::register_cvar_change_listeners() {
        show_frametime_display->on_change([&](Rx::Console::Variable<bool>& var) {
            if(var) {
                make_frametime_display();
            } else {
                destroy_frametime_display();
            }
        });

        show_console->on_change([&](Rx::Console::Variable<bool>& var) {
            if(var) {
                make_console_window();
            } else {
                destroy_console_window();
            }
        });
    }

    void SanityEngine::create_planetary_atmosphere() {
        auto locked_registry = registry.lock();
        const auto atmosphere = locked_registry->create();

        // No need to set parameters, the default light component represents the Earth's sun
        locked_registry->emplace<renderer::LightComponent>(atmosphere);
        locked_registry->emplace<renderer::AtmosphericSkyComponent>(atmosphere);
        locked_registry->emplace<TransformComponent>(atmosphere); // Light rotations come from a Transform

        // Camera for the directional light's shadow
        // TODO: Set this up as orthographic? Or maybe a separate component for shadow cameras?
        auto& shadow_camera = locked_registry->emplace<renderer::CameraComponent>(atmosphere);
        shadow_camera.aspect_ratio = 1;
        shadow_camera.fov = 0;
    }

    void SanityEngine::make_frametime_display() {
        if(!frametime_display_entity) {
            auto locked_registry = registry.lock();
            frametime_display_entity = locked_registry->create();
            locked_registry->emplace<ui::UiComponent>(*frametime_display_entity,
                                                      Rx::make_ptr<ui::FramerateDisplay>(RX_SYSTEM_ALLOCATOR, framerate_tracker));
        }
    }

    void SanityEngine::destroy_frametime_display() {
        if(frametime_display_entity) {
            auto locked_registry = registry.lock();
            locked_registry->destroy(*frametime_display_entity);
        }
    }

    void SanityEngine::make_console_window() {
        if(!console_window_entity) {
            auto locked_registry = registry.lock();
            console_window_entity = locked_registry->create();
            locked_registry->emplace<ui::UiComponent>(*console_window_entity,
                                                      Rx::make_ptr<ui::ConsoleWindow>(RX_SYSTEM_ALLOCATOR, console_context));
        }
    }

    void SanityEngine::destroy_console_window() {
        if(console_window_entity) {
            auto locked_registry = registry.lock();
            locked_registry->destroy(*console_window_entity);
        }
    }

    void SanityEngine::create_first_person_player() {
        auto locked_registry = registry.lock();
        player = locked_registry->create();

        auto& transform = locked_registry->emplace<TransformComponent>(player);
        transform.location.z = 5;
        transform.location.y = 2;
        transform.rotation = glm::angleAxis(0.0f, glm::vec3{1, 0, 0});
        locked_registry->emplace<renderer::CameraComponent>(player);

        // player_controller = Rx::make_ptr<FirstPersonController>(RX_SYSTEM_ALLOCATOR, window, player, registry);

        logger->info("Created flycam");
    }

    void SanityEngine::create_environment_object_editor() {
        // auto locked_registry = registry.lock();
        // const auto entity = locked_registry->create();
        // auto& ui_panel = locked_registry->assign<ui::UiComponent>(entity);
        //
        // auto* handle = scripting_runtime->instantiate_script_object("terraingen", "EnvironmentObjectEditor");
        // ui_panel.panel = Rx::make_ptr<ui::ScriptedUiPanel>(RX_SYSTEM_ALLOCATOR, handle, *scripting_runtime);
    }

    void SanityEngine::load_3d_object(const Rx::String& filename) {
        const auto msg = Rx::String::format("load_3d_object(%s)", filename);
        ZoneScopedN(msg.data());
        load_static_mesh(filename, registry, *renderer);
    }

    void SanityEngine::render() {
        auto locked_registry = registry.lock();

        imgui_adapter->draw_ui(locked_registry->view<ui::UiComponent>());

        renderer->render_all(locked_registry, *world);

        renderer->end_frame();

        FrameMark;
#ifdef TRACY_ENABLE
        TracyD3D12NewFrame(renderer::RenderBackend::tracy_context);
#endif

        TracyD3D12Collect(renderer::RenderBackend::tracy_context);
    }

    void initialize_g_engine(const char* executable_directory) { g_engine = new SanityEngine{executable_directory}; }
} // namespace sanity::engine
