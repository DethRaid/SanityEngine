#include "sanity_engine.hpp"

#define STB_IMAGE_IMPLEMENTATION

#include <filesystem>
#include <glm/ext/quaternion_trigonometric.inl>

#include <GLFW/glfw3.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <minitrace.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <stb_image.h>

#include "core/abort.hpp"
#include "loading/entity_loading.hpp"
#include "loading/image_loading.hpp"
#include "renderer/standard_material.hpp"
#include "rhi/render_device.hpp"
#include "ui/fps_display.hpp"
#include "ui/ui_components.hpp"

struct AtmosphereMaterial {
    glm::vec3 sun_vector;
};

int main() {
    Settings settings{};
    // settings.enable_gpu_crash_reporting = true;

    SanityEngine engine{settings};

    engine.run();

    spdlog::warn("REMAIN INDOORS");
}

static void error_callback(const int error, const char* description) { spdlog::error("{} (GLFW error {}}", description, error); }

static void key_func(GLFWwindow* window, const int key, int /* scancode */, const int action, const int mods) {
    auto* input_manager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

    input_manager->on_key(key, action, mods);
}

SanityEngine::SanityEngine(const Settings& settings_in)
    : logger{spdlog::stdout_color_st("SanityEngine")}, settings{settings_in}, input_manager{std::make_unique<InputManager>()} {
    mtr_init("SanityEngine.json");

    MTR_SCOPE("SanityEngine", "SanityEngine");

    // spdlog::set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] %v");
    spdlog::set_pattern("[%n] [%^%l%$] %v");

    logger->set_level(spdlog::level::debug);

    logger->info("HELLO HUMAN");

    task_scheduler = std::make_unique<ftl::TaskScheduler>();
    task_scheduler->Init();

    task_scheduler->SetEmptyQueueBehavior(ftl::EmptyQueueBehavior::Sleep);

    {
        MTR_SCOPE("SanityEngine", "glfwInit");
        if(!glfwInit()) {
            critical_error("Could not initialize GLFW");
        }
    }

    glfwSetErrorCallback(error_callback);

    {
        MTR_SCOPE("SanityEngine", "glfwCreateWindow");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(1000, 480, "Sanity Engine", nullptr, nullptr);
        if(window == nullptr) {
            critical_error("Could not create GLFW window");
        }
    }

    logger->info("Created window");

    glfwSetWindowUserPointer(window, input_manager.get());
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetKeyCallback(window, key_func);

    renderer = std::make_unique<renderer::Renderer>(window, settings);
    logger->info("Initialized renderer");

    bve = std::make_unique<BveWrapper>(renderer->get_render_device());

    create_flycam_player();

    create_planetary_atmosphere();

    make_frametime_display();

    imgui_adapter = std::make_unique<DearImguiAdapter>(window, *renderer);

    world = World::create({.seed = static_cast<uint32_t>(rand()),
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
}

SanityEngine::~SanityEngine() {
    glfwDestroyWindow(window);

    glfwTerminate();

    mtr_shutdown();
}

void SanityEngine::run() {
    uint64_t frame_count = 1;

    float last_frame_duration = 0;

    while(!glfwWindowShouldClose(window)) {
        {
            MTR_SCOPE("SanityEngine", "tick");
            const auto frame_start_time = std::chrono::steady_clock::now();
            glfwPollEvents();

            player_controller->update_player_transform(last_frame_duration);

            imgui_adapter->draw_ui(registry.view<ui::UiComponent>());

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

            renderer->render_all(registry);

            renderer->end_frame();

            const auto frame_end_time = std::chrono::steady_clock::now();

            const auto microsecond_frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_end_time - frame_start_time)
                                                        .count();
            last_frame_duration = static_cast<float>(static_cast<double>(microsecond_frame_duration) / 1000000.0);

            framerate_tracker.add_frame_time(last_frame_duration);

            frame_count++;
        }

        mtr_flush();
    }
}

void SanityEngine::create_debug_plane() {
    const auto vertices = std::vector<StandardVertex>{
        {/* .position = */ {-5, -1, 5}, /* .normal = */ {0, 1, 0}, /* .color = */ 0xFF727272, /* .texcoord = */ {}},
        {/* .position = */ {5, -1, -5}, /* .normal = */ {0, 1, 0}, /* .color = */ 0xFF727272, /* .texcoord = */ {}},
        {/* .position = */ {5, -1, 5}, /* .normal = */ {0, 1, 0}, /* .color = */ 0xFF727272, /* .texcoord = */ {}},
        {/* .position = */ {-5, -1, -5}, /* .normal = */ {0, 1, 0}, /* .color = */ 0xFF727272, /* .texcoord = */ {}},
    };

    const auto indices = std::vector<uint32_t>{0, 1, 2, 0, 3, 1};

    auto& device = renderer->get_render_device();
    auto commands = device.create_command_list();

    auto& mesh_store = renderer->get_static_mesh_store();

    mesh_store.begin_adding_meshes(commands);
    auto plane_renderable = mesh_store.add_mesh(vertices, indices, commands);
    mesh_store.end_adding_meshes(commands);

    device.submit_command_list(std::move(commands));

    const auto plane_entity = registry.create();

    registry.assign<renderer::StaticMeshRenderableComponent>(plane_entity, std::move(plane_renderable));

    logger->info("Created plane");
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
    registry.assign<ui::UiComponent>(frametime_display, std::make_unique<ui::FramerateDisplay>(framerate_tracker));
}

void SanityEngine::create_flycam_player() {
    player = registry.create();

    auto& transform = registry.assign<TransformComponent>(player);
    transform.position.z = 5;
    transform.position.y = 2;
    transform.rotation = glm::angleAxis(0.0f, glm::vec3{1, 0, 0});
    registry.assign<renderer::CameraComponent>(player);

    player_controller = std::make_unique<FirstPersonController>(window, player, registry);

    logger->info("Created flycam");
}

void SanityEngine::load_bve_train(const std::string& filename) {
    const auto success = bve->add_train_to_scene(filename, registry, *renderer);
    if(!success) {
        logger->error("Could not load train file {}", filename);
    }
}

void SanityEngine::load_3d_object(const std::string& filename) {
    const auto msg = fmt::format("load_3d_object({})", filename);
    MTR_SCOPE("SanityEngine", msg.c_str());
    load_static_mesh(filename, registry, *renderer);
}
