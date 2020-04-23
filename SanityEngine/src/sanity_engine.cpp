#include "sanity_engine.hpp"

#include <GLFW/glfw3.h>
#include <minitrace.h>
#include <spdlog/spdlog.h>
#include <time.h>

#include "core/abort.hpp"
#include "debugging/renderdoc.hpp"

int main() {
    SanityEngine engine;

    engine.run();

    spdlog::warn("REMAIN INDOORS");
}

static void error_callback(const int error, const char* description) { spdlog::error("{} (GLFW error {}}", description, error); }

SanityEngine::SanityEngine() {
    mtr_init("SanityEngine.json");

    MTR_SCOPE("D3D12Engine", "D3D12Engine");

    spdlog::info("HELLO HUMAN");

    if(settings.enable_renderdoc) {
        renderdoc = load_renderdoc("C:/Users/gold1/bin/RenderDoc/RenderDoc_2020_02_06_fe30fa91_64/renderdoc.dll");
    }

    if(!glfwInit()) {
        critical_error("Could not initialize GLFW");
    }

    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(640, 480, "Sanity Engine", nullptr, nullptr);
    if(window == nullptr) {
        critical_error("Could not create GLFW window");
    }

    spdlog::info("Created window");

    renderer = std::make_unique<renderer::Renderer>(window);
    spdlog::info("Initialized renderer");

    create_debug_cube();
}

SanityEngine::~SanityEngine() {
    glfwDestroyWindow(window);

    glfwTerminate();

    mtr_flush();
}

void SanityEngine::run() {
    double last_frame_duration = 0;

    while(!glfwWindowShouldClose(window)) {
        const auto start_time = clock();

        glfwPollEvents();

        tick(last_frame_duration);

        const auto end_time = clock();

        last_frame_duration = static_cast<double>(end_time - start_time) / static_cast<double>(CLOCKS_PER_SEC);

        mtr_flush();
    }
}

void SanityEngine::create_debug_cube() {
    const auto vertices = std::vector<BveVertex>{
        // Front
        {/* .position = */ {-1, -1, 1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
        {/* .position = */ {-1, 1, 1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
        {/* .position = */ {1, 1, 1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
        {/* .position = */ {1, -1, 1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},

        // Right
        {/* .position = */ {1, -1, 1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
        {/* .position = */ {1, 1, 1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
        {/* .position = */ {1, 1, -1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
        {/* .position = */ {1, -1, -1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},

        // Back
        {/* .position = */ {1, -1, -1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
        {/* .position = */ {1, 1, -1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
        {/* .position = */ {-1, 1, -1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
        {/* .position = */ {-1, -1, -1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},

        // Left
        {/* .position = */ {-1, -1, -1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
        {/* .position = */ {-1, 1, -1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
        {/* .position = */ {-1, 1, 1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
        {/* .position = */ {-1, -1, 1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},

        // Top
        {/* .position = */ {-1, 1, 1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
        {/* .position = */ {-1, -1, 1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
        {/* .position = */ {1, -1, 1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
        {/* .position = */ {1, 1, 1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},

        // Bottom
        {/* .position = */ {1, -1, -1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
        {/* .position = */ {1, 1, -1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
        {/* .position = */ {-1, 1, -1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
        {/* .position = */ {-1, -1, -1}, /* .normal = */ {}, /* .color = */ {}, /* .texcoord = */ {}, /* .double_sided = */ {}},
    };

    const auto indices = std::vector<uint32_t>{// Front
                                               0,
                                               1,
                                               2,
                                               0,
                                               2,
                                               3,

                                               // Right
                                               4,
                                               5,
                                               6,
                                               4,
                                               6,
                                               7,

                                               // Back
                                               8,
                                               9,
                                               10,
                                               8,
                                               10,
                                               11,

                                               // Left
                                               12,
                                               13,
                                               14,
                                               12,
                                               14,
                                               15,

                                               // Top
                                               16,
                                               17,
                                               18,
                                               16,
                                               18,
                                               19,

                                               // Bottom
                                               20,
                                               21,
                                               22,
                                               20,
                                               22,
                                               23};

    auto cube_renderable = renderer->create_static_mesh(vertices, indices);

    const auto cube_entity = registry.create();

    registry.emplace<renderer::StaticMeshRenderable>(cube_entity, cube_renderable);
}

void SanityEngine::tick(double delta_time) {
    MTR_SCOPE("D3D12Engine", "tick");

    renderer->render_scene(registry);
}
