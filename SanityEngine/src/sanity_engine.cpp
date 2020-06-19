#include "sanity_engine.hpp"

#define STB_IMAGE_IMPLEMENTATION

#include <filesystem>
#include <glm/ext/quaternion_trigonometric.inl>

#include <GLFW/glfw3.h>
#include <rx/core/abort.h>
#include <rx/core/log.h>
#include <stb_image.h>
#include <winrt/Windows.Foundation.h>

#include "adapters/tracy.hpp"
#include "globals.hpp"
#include "loading/entity_loading.hpp"
#include "rhi/render_device.hpp"
#include "ui/fps_display.hpp"
#include "ui/ui_components.hpp"
#include "world/world.hpp"

static Rx::GlobalGroup s_sanity_engine_globals{"SanityEngine"};

static Rx::Global<ftl::TaskScheduler> task_scheduler{"SanityEngine", "TaskScheduler"};

RX_LOG("SanityEngine", logger);

struct AtmosphereMaterial {
    glm::vec3 sun_vector;
};

int main() {
    const Settings settings{};
    // settings.enable_gpu_crash_reporting = true;

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
    : settings{settings_in}, input_manager{Rx::make_ptr<InputManager>(Rx::Memory::SystemAllocator::instance())} {

    ZoneScoped;

    winrt::init_apartment();

    logger->info("HELLO HUMAN");

    task_scheduler->Init();

    task_scheduler->SetEmptyQueueBehavior(ftl::EmptyQueueBehavior::Sleep);

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

    renderer = Rx::make_ptr<renderer::Renderer>(Rx::Memory::SystemAllocator::instance(), window, settings);
    logger->info("Initialized renderer");

    // initialize_scripting_runtime();

    bve = Rx::make_ptr<BveWrapper>(Rx::Memory::SystemAllocator::instance(), renderer->get_render_device());

    create_first_person_player();

    create_planetary_atmosphere();

    make_frametime_display();

    imgui_adapter = Rx::make_ptr<DearImguiAdapter>(Rx::Memory::SystemAllocator::instance(), window, *renderer);

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

    world->tick(0);

    load_3d_object("data/models/davifactory.obj");

    // create_simple_boi(registry, *scripting_runtime);
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

        imgui_adapter->draw_ui(registry.view<ui::UiComponent>());

        const auto thread_idx = task_scheduler->GetCurrentThreadIndex();

        // Renderer MUST begin the frame before any tasks that potentially do render-related things like data streaming, terrain
        // generation, etc
        renderer->begin_frame(frame_count, thread_idx);

        // There might not be a world if the player is still in the main menu
        if(world) {
            world->tick(last_frame_duration);
        }

        if(frame_count == 1) {
            // load_bve_train("data/bve_trains/R46 2014 (8 Car)/Cars/Body/BodyA.b3d");
        }

        renderer->render_all(registry, *world);

        renderer->end_frame(thread_idx);

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

entt::registry& SanityEngine::get_registry() { return registry; }

World* SanityEngine::get_world() const { return world.get(); }

void SanityEngine::initialize_scripting_runtime() {
    scripting_runtime = horus::ScriptingRuntime::create(registry);
    if(!scripting_runtime) {
        Rx::abort("Could not initialize scripting runtime");
    }

    register_horus_api();

    const auto success = scripting_runtime->add_script_directory(R"(E:\Documents\SanityEngine\SanityEngine\scripts)");
    if(!success) {
        Rx::abort("Could not register SanityEngine builtin scripts modifier");
    }
}

void SanityEngine::register_horus_api() const {
    /*
     * Everything in this method is auto-generated when the code is re-built. You should not put any code you care about in this method, nor
     * should you modify the code in this method in any way
     */

    _scripting_entity_scripting_api_register_with_scripting_runtime(*scripting_runtime);
}

void SanityEngine::create_planetary_atmosphere() {
    const auto atmosphere = registry.create();

    // No need to set parameters, the default light component represents the Earth's sun
    registry.assign<renderer::LightComponent>(atmosphere);
    registry.assign<renderer::AtmosphericSkyComponent>(atmosphere);
    registry.assign<TransformComponent>(atmosphere); // Light rotations come from a Transform

    // Camera for the directional light's shadow
    // TODO: Set this up as orthographic? Or maybe a separate component for shadow cameras?
    auto& shadow_camera = registry.assign<renderer::CameraComponent>(atmosphere);
    shadow_camera.aspect_ratio = 1;
    shadow_camera.fov = 0;
}

void SanityEngine::make_frametime_display() {
    const auto frametime_display = registry.create();
    registry.assign<ui::UiComponent>(frametime_display,
                                     Rx::make_ptr<ui::FramerateDisplay>(Rx::Memory::SystemAllocator::instance(), framerate_tracker));
}

void SanityEngine::create_first_person_player() {
    player = registry.create();

    auto& transform = registry.assign<TransformComponent>(player);
    transform.location.z = 5;
    transform.location.y = 2;
    transform.rotation = glm::angleAxis(0.0f, glm::vec3{1, 0, 0});
    registry.assign<renderer::CameraComponent>(player);

    player_controller = Rx::make_ptr<FirstPersonController>(Rx::Memory::SystemAllocator::instance(), window, player, registry);

    logger->info("Created flycam");
}

void SanityEngine::load_bve_train(const Rx::String& filename) {
    const auto success = bve->add_train_to_scene(filename, registry, *renderer);
    if(!success) {
        logger->error("Could not load train file {}", filename.data());
    }
}

void SanityEngine::load_3d_object(const Rx::String& filename) {
    const auto msg = Rx::String::format("load_3d_object(%s)", filename);
    ZoneScopedN(msg.data(), SUBSYSTEMS_TO_PROFILE & SubsystemStreaming);
    load_static_mesh(filename, registry, *renderer);
}
