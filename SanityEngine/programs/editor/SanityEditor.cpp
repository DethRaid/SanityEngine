#include "SanityEditor.hpp"

#include "rx/core/abort.h"
#include "rx/core/log.h"
#include "sanity_engine.hpp"
#include "ui/ApplicationGui.hpp"
#include "ui/ui_components.hpp"

RX_LOG("SanityEditor", logger);

int main(int argc, char** argv) {
    initialize_g_engine(R"(E:\Documents\SanityEngine\x64\Debug)");

    auto editor = sanity::editor::SanityEditor{R"(E:\Documents\SanityEngine\x64\Debug)"};

    editor.run_until_quit();

    return 0;
}

namespace sanity::editor {
    SanityEditor::SanityEditor(const char* executable_directory) {}

    void SanityEditor::run_until_quit() {
        auto* window = g_engine->get_window();

        while(glfwWindowShouldClose(window) == GLFW_FALSE) {
            glfwPollEvents();

            g_engine->tick();
        }
    }

    void SanityEditor::create_application_gui() {
        auto registry = g_engine->get_registry().lock();

        const auto application_gui_entity = registry->create();
        registry->emplace<::ui::UiComponent>(application_gui_entity, Rx::make_ptr<ui::ApplicationGui>(RX_SYSTEM_ALLOCATOR, ui_controller));
    }
} // namespace sanity::editor
