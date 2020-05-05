﻿#include "sanity_engine.hpp"

#include <GLFW/glfw3.h>
#include <minitrace.h>
#include <glm/ext/quaternion_trigonometric.inl>
#include <spdlog/spdlog.h>

#include "core/abort.hpp"

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

SanityEngine::SanityEngine(const Settings& settings_in) : settings{settings_in}, input_manager{std::make_unique<InputManager>()} {
    mtr_init("SanityEngine.json");

    MTR_SCOPE("SanityEngine", "SanityEngine");

    // spdlog::set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] %v");
    spdlog::set_pattern("[%n] [%^%l%$] %v");

    spdlog::info("HELLO HUMAN");

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

    spdlog::info("Created window");

    glfwSetWindowUserPointer(window, input_manager.get());
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetKeyCallback(window, key_func);

    renderer = std::make_unique<renderer::Renderer>(window, settings);
    spdlog::info("Initialized renderer");
}

SanityEngine::~SanityEngine() {
    glfwDestroyWindow(window);

    glfwTerminate();

    mtr_shutdown();
}

void SanityEngine::run() {
    auto clock = std::chrono::high_resolution_clock{};
    uint64_t frame_count = 1;

    float last_frame_duration = 0;

    while(!glfwWindowShouldClose(window)) {
        {
            MTR_SCOPE("SanityEngine", "tick");
            const auto frame_start_time = clock.now();
            glfwPollEvents();
            renderer->begin_frame(frame_count);

            // Hackily spawn entities on the first frame, because mesh uploading is hard
            if(frame_count == 1) {
                create_debug_plane();

                create_debug_cube();

                create_flycam_player();
            }

            player_controller->update_player_transform(last_frame_duration);

            renderer->render_scene(registry);

            renderer->end_frame();

            const auto frame_end_time = clock.now();

            const auto microsecond_frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(frame_end_time - frame_start_time)
                                                        .count();
            last_frame_duration = static_cast<float>(static_cast<double>(microsecond_frame_duration) / 1000000.0);

            framerate_tracker.add_frame_time(last_frame_duration);

            if(frame_count % 100 == 0) {
                spdlog::info("Frame {} stats", frame_count);
                framerate_tracker.log_framerate_stats();
            }

            frame_count++;
        }

        mtr_flush();
    }
}

void SanityEngine::create_debug_cube() {
    const auto vertices = std::vector<BveVertex>{
        // Front
        {/* .position = */ {-0.5f, 0.5f, 0.5f}, /* .normal = */ {0, 0, 1}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
        {/* .position = */ {0.5f, -0.5f, 0.5f}, /* .normal = */ {0, 0, 1}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
        {/* .position = */ {-0.5f, -0.5f, 0.5f}, /* .normal = */ {0, 0, 1}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
        {/* .position = */ {0.5f, 0.5f, 0.5f}, /* .normal = */ {0, 0, 1}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},

        // Right
        {/* .position = */ {-0.5f, -0.5f, -0.5f}, /* .normal = */ {-1, 0, 0}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
        {/* .position = */ {-0.5f, 0.5f, 0.5f}, /* .normal = */ {-1, 0, 0}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
        {/* .position = */ {-0.5f, -0.5f, 0.5f}, /* .normal = */ {-1, 0, 0}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
        {/* .position = */ {-0.5f, 0.5f, -0.5f}, /* .normal = */ {-1, 0, 0}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},

        // Left
        {/* .position = */ {0.5f, 0.5f, 0.5f}, /* .normal = */ {1, 0, 0}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
        {/* .position = */ {0.5f, -0.5f, -0.5f}, /* .normal = */ {1, 0, 0}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
        {/* .position = */ {0.5f, -0.5f, 0.5f}, /* .normal = */ {1, 0, 0}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
        {/* .position = */ {0.5f, 0.5f, -0.5f}, /* .normal = */ {1, 0, 0}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},

        // Back
        {/* .position = */ {0.5f, 0.5f, -0.5f}, /* .normal = */ {0, 0, -1}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
        {/* .position = */ {-0.5f, -0.5f, -0.5f}, /* .normal = */ {0, 0, -1}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
        {/* .position = */ {0.5f, -0.5f, -0.5f}, /* .normal = */ {0, 0, -1}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
        {/* .position = */ {-0.5f, 0.5f, -0.5f}, /* .normal = */ {0, 0, -1}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},

        // Top
        {/* .position = */ {-0.5f, 0.5f, -0.5f}, /* .normal = */ {0, 1, 0}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
        {/* .position = */ {0.5f, 0.5f, 0.5f}, /* .normal = */ {0, 1, 0}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
        {/* .position = */ {0.5f, 0.5f, -0.5f}, /* .normal = */ {0, 1, 0}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
        {/* .position = */ {-0.5f, 0.5f, 0.5f}, /* .normal = */ {0, 1, 0}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},

        // Bottom
        {/* .position = */ {0.5f, -0.5f, 0.5f}, /* .normal = */ {0, -1, 0}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
        {/* .position = */ {-0.5f, -0.5f, -0.5f}, /* .normal = */ {0, -1, 0}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
        {/* .position = */ {0.5f, -0.5f, -0.5f}, /* .normal = */ {0, -1, 0}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
        {/* .position = */ {-0.5f, -0.5f, 0.5f}, /* .normal = */ {0, -1, 0}, /* .color = */ 0xFFCDCDCD, /* .texcoord = */ {}},
    };

    const auto indices = std::vector<uint32_t>{
        // front face
        0,
        1,
        2, // first triangle
        0,
        3,
        1, // second triangle

        // left face
        4,
        5,
        6, // first triangle
        4,
        7,
        5, // second triangle

        // right face
        8,
        9,
        10, // first triangle
        8,
        11,
        9, // second triangle

        // back face
        12,
        13,
        14, // first triangle
        12,
        15,
        13, // second triangle

        // top face
        16,
        17,
        18, // first triangle
        16,
        19,
        17, // second triangle

        // bottom face
        20,
        21,
        22, // first triangle
        20,
        23,
        21, // second triangle
    };

    auto cube_renderable = renderer->create_static_mesh(vertices, indices);

    const auto cube_entity = registry.create();

    registry.assign<renderer::StaticMeshRenderableComponent>(cube_entity, cube_renderable);

    spdlog::info("Created cube");
}

void SanityEngine::create_debug_plane() {
    const auto vertices = std::vector<BveVertex>{
        {/* .position = */ {-5, -1, 5}, /* .normal = */ {0, 1, 0}, /* .color = */ 0xFF727272, /* .texcoord = */ {}},
        {/* .position = */ {5, -1, -5}, /* .normal = */ {0, 1, 0}, /* .color = */ 0xFF727272, /* .texcoord = */ {}},
        {/* .position = */ {5, -1, 5}, /* .normal = */ {0, 1, 0}, /* .color = */ 0xFF727272, /* .texcoord = */ {}},
        {/* .position = */ {-5, -1, -5}, /* .normal = */ {0, 1, 0}, /* .color = */ 0xFF727272, /* .texcoord = */ {}},
    };

    const auto indices = std::vector<uint32_t>{0, 1, 2, 0, 3, 1};

    auto plane_renderable = renderer->create_static_mesh(vertices, indices);

    const auto plane_entity = registry.create();

    registry.assign<renderer::StaticMeshRenderableComponent>(plane_entity, plane_renderable);

    spdlog::info("Created plane");
}

void SanityEngine::create_flycam_player() {
    player = registry.create();

    auto& transform = registry.assign<TransformComponent>(player);
    transform.position.z = 5;
    transform.rotation = glm::angleAxis(0.0f, glm::vec3{1, 0, 0});
    registry.assign<renderer::CameraComponent>(player);

    player_controller = std::make_unique<FlycamController>(window, player, registry);

    spdlog::info("Created flycam");
}
