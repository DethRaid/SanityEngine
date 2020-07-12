#include "sanity_engine.hpp"

#define STB_IMAGE_IMPLEMENTATION

#include <filesystem>

#include <winrt/Windows.Foundation.h>

#include "GLFW/glfw3.h"
#include "TracyD3D12.hpp"
#include "adapters/tracy.hpp"
#include "glm/ext/quaternion_trigonometric.inl"
#include "globals.hpp"
#include "loading/entity_loading.hpp"
#include "rhi/render_device.hpp"
#include "rx/core/abort.h"
#include "rx/core/log.h"
#include "stb_image.h"
#include "ui/fps_display.hpp"
#include "ui/scripted_ui_panel.hpp"
#include "ui/ui_components.hpp"
#include "world/generation/gpu_terrain_generation.hpp"
#include "world/world.hpp"

static Rx::GlobalGroup s_sanity_engine_globals{"SanityEngine"};

RX_LOG("SanityEngine", logger);

struct AtmosphereMaterial {
    glm::vec3 sun_vector;
};

int main() {
    winrt::init_apartment();

    const Settings settings{};

    g_engine = new SanityEngine{settings};

    g_engine->run();

    logger->warning("REMAIN INDOORS");

    delete g_engine;
}

static void error_callback(const int error, const char* description) { logger->error("%s (GLFW error %d}", description, error); }

static void key_func(GLFWwindow* window, const int key, int /* scancode */, const int action, const int mods) {
    auto* input_manager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

    input_manager->on_key(key, action, mods);
}

SanityEngine::SanityEngine(const Settings& settings_in)
    : settings{settings_in}, input_manager{Rx::make_ptr<InputManager>(RX_SYSTEM_ALLOCATOR)} {
    logger->info("HELLO HUMAN");

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
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        glfwSetKeyCallback(window, key_func);

        renderer = Rx::make_ptr<renderer::Renderer>(RX_SYSTEM_ALLOCATOR, window, settings);
        logger->info("Initialized renderer");

        initialize_scripting_runtime();

        bve = Rx::make_ptr<BveWrapper>(RX_SYSTEM_ALLOCATOR, renderer->get_render_device());

        create_first_person_player();

        create_planetary_atmosphere();

        make_frametime_display();

        imgui_adapter = Rx::make_ptr<DearImguiAdapter>(RX_SYSTEM_ALLOCATOR, window, *renderer);

        terraingen::initialize(renderer->get_render_device());

        world = World::create({.seed = 666,
                               .height = 128,
                               .width = 128,
                               .max_ocean_depth = 8,
                               .min_terrain_depth_under_ocean = 8,
                               .max_height_above_sea_level = 16},
                              player,
                              registry,
                              *renderer);

        player_controller->set_current_terrain(world->get_terrain());

        create_environment_object_editor();

        world->tick(0);

        load_3d_object("data/models/davifactory.obj");
    }
}

SanityEngine::~SanityEngine() {
    glfwDestroyWindow(window);

    glfwTerminate();
}

void SanityEngine::run() {
    uint64_t frame_count = 1;

    Float32 last_frame_duration = 0;

    while(!glfwWindowShouldClose(window)) {
        ZoneScopedN("tick");
        const auto frame_start_time = std::chrono::steady_clock::now();
        glfwPollEvents();

        player_controller->update_player_transform(last_frame_duration);

        {
            auto locked_registry = registry.lock();
            imgui_adapter->draw_ui(locked_registry->view<ui::UiComponent>());
        }

        // Renderer MUST begin the frame before any tasks that potentially do render-related things like data streaming, terrain
        // generation, etc
        renderer->begin_frame(frame_count);

        // There might not be a world if the player is still in the main menu
        if(world) {
            world->tick(last_frame_duration);
        }

        if(frame_count == 1) {
            // load_bve_train("data/bve_trains/R46 2014 (8 Car)/Cars/Body/BodyA.b3d");
        }

        renderer->render_all(registry.lock(), *world);

        renderer->end_frame();

        FrameMark;
#ifdef TRACY_ENABLE
        TracyD3D12NewFrame(renderer::RenderDevice::tracy_context);
#endif

        const auto frame_end_time = std::chrono::steady_clock::now();

        const auto microsecond_frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_end_time - frame_start_time)
                                                    .count();
        last_frame_duration = static_cast<Float32>(static_cast<double>(microsecond_frame_duration) / 1000000.0);

        framerate_tracker.add_frame_time(last_frame_duration);

        frame_count++;

        TracyD3D12Collect(renderer::RenderDevice::tracy_context);
    }
}

entt::entity SanityEngine::get_player() const { return player; }

SynchronizedResource<entt::registry>& SanityEngine::get_registry() { return registry; }

World* SanityEngine::get_world() const { return world.get(); }

void SanityEngine::initialize_scripting_runtime() {
    scripting_runtime = Rx::make_ptr<coreclr::Host>(RX_SYSTEM_ALLOCATOR,
                                                    R"(C:\Program Files\dotnet\shared\Microsoft.NETCore.App\5.0.0-preview.4.20251.6)");
    if(!scripting_runtime) {
        Rx::abort("Could not initialize scripting runtime");
    }
}

void SanityEngine::create_planetary_atmosphere() {
    auto locked_registry = registry.lock();
    const auto atmosphere = locked_registry->create();

    // No need to set parameters, the default light component represents the Earth's sun
    locked_registry->assign<renderer::LightComponent>(atmosphere);
    locked_registry->assign<renderer::AtmosphericSkyComponent>(atmosphere);
    locked_registry->assign<TransformComponent>(atmosphere); // Light rotations come from a Transform

    // Camera for the directional light's shadow
    // TODO: Set this up as orthographic? Or maybe a separate component for shadow cameras?
    auto& shadow_camera = locked_registry->assign<renderer::CameraComponent>(atmosphere);
    shadow_camera.aspect_ratio = 1;
    shadow_camera.fov = 0;
}

void SanityEngine::make_frametime_display() {
    auto locked_registry = registry.lock();
    const auto frametime_display = locked_registry->create();
    locked_registry->assign<ui::UiComponent>(frametime_display, Rx::make_ptr<ui::FramerateDisplay>(RX_SYSTEM_ALLOCATOR, framerate_tracker));
}

void SanityEngine::create_first_person_player() {
    auto locked_registry = registry.lock();
    player = locked_registry->create();

    auto& transform = locked_registry->assign<TransformComponent>(player);
    transform.location.z = 5;
    transform.location.y = 2;
    transform.rotation = glm::angleAxis(0.0f, glm::vec3{1, 0, 0});
    locked_registry->assign<renderer::CameraComponent>(player);

    player_controller = Rx::make_ptr<FirstPersonController>(RX_SYSTEM_ALLOCATOR, window, player, registry);

    logger->info("Created flycam");
}

void SanityEngine::load_bve_train(const Rx::String& filename) {
    const auto success = bve->add_train_to_scene(filename, registry, *renderer);
    if(!success) {
        logger->error("Could not load train file {}", filename.data());
    }
}

void SanityEngine::create_environment_object_editor() {
    // auto locked_registry = registry.lock();
    // const auto entity = locked_registry->create();
    // auto& ui_panel = locked_registry->assign<ui::UiComponent>(entity);
    //
    // auto* handle = scripting_runtime->instantiate_script_object("engine/terraingen", "EnvironmentObjectEditor");
    // ui_panel.panel = Rx::make_ptr<ui::ScriptedUiPanel>(RX_SYSTEM_ALLOCATOR, handle, *scripting_runtime);
}

void SanityEngine::load_3d_object(const Rx::String& filename) {
    const auto msg = Rx::String::format("load_3d_object(%s)", filename);
    ZoneScopedN(msg.data());
    load_static_mesh(filename, registry, *renderer);
}
