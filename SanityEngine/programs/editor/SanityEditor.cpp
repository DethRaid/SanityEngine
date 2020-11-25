#include "SanityEditor.hpp"

#include "rx/core/abort.h"
#include "rx/core/log.h"
#include "sanity_engine.hpp"

RX_LOG("SanityEditor", logger);

int main(int argc, char** argv) {
    initialize_g_engine(R"(E:\Documents\SanityEngine\x64\Debug)");

    auto editor = sanity::editor::SanityEditor{R"(E:\Documents\SanityEngine\x64\Debug)"};

    editor.run_until_quit();

    return 0;
}

namespace sanity::editor {
    SanityEditor::SanityEditor(const char* executable_directory) { create_editor_ui(); }

    void SanityEditor::run_until_quit() const {
        auto* window = g_engine->get_window();

        while(glfwWindowShouldClose(window) == GLFW_FALSE) {
            glfwPollEvents();

            g_engine->tick();
        }
    }

    void SanityEditor::create_editor_ui() const {
        auto& registry_synchronizer = g_engine->get_registry();
        auto registry = registry_synchronizer.lock();
    }
} // namespace sanity::editor
