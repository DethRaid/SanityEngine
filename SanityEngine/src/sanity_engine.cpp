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
}

static void error_callback(const int error, const char* description) { spdlog::error("{} (GLFW error {}}", description, error); }

SanityEngine::SanityEngine() {
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
    window = glfwCreateWindow(640, 480, "D3D12 Engine", nullptr, nullptr);
    if(window == nullptr) {
        critical_error("Could not create GLFW window");
    }

    renderer = std::make_unique<renderer::Renderer>(window);

    create_debug_cube();
}

SanityEngine::~SanityEngine() {
    glfwDestroyWindow(window);

    glfwTerminate();

    spdlog::warn("REMAIN INDOORS");

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
    auto vertices = std::vector<BveVertex>{{/* .position = */ /* .normal = */ /* .color = */ /* .texcoord = */ /* .double_sided = */}};
}

void SanityEngine::tick(double delta_time) {
    MTR_SCOPE("D3D12Engine", "tick");

    renderer->render_scene(registry);
}
