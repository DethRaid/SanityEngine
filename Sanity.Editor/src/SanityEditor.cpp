#include "SanityEditor.hpp"

#include "rx/core/abort.h"
#include "rx/core/log.h"
#include "sanity_engine.hpp"
#include "ui/ApplicationGui.hpp"
#include "ui/ui_components.hpp"

RX_LOG("SanityEditor", logger);

int main(int argc, char** argv) {
    sanity::engine::initialize_g_engine(R"(E:\Documents\SanityEngine\x64\Debug)");

    auto editor = sanity::editor::SanityEditor{R"(E:\Documents\SanityEngine\x64\Debug)"};

    editor.run_until_quit();

    return 0;
}

namespace sanity::editor {
    SanityEditor::SanityEditor(const char* executable_directory)
        : flycam{engine::g_engine->get_window(), engine::g_engine->get_player(), engine::g_engine->get_global_registry()} {
        create_application_gui();
    }

    void SanityEditor::run_until_quit() {
        auto* window = engine::g_engine->get_window();

        while(glfwWindowShouldClose(window) == GLFW_FALSE) {
            glfwPollEvents();

            engine::g_engine->do_frame();
        }
    }

    void SanityEditor::create_application_gui() {
        auto registry = engine::g_engine->get_global_registry().lock();

        const auto application_gui_entity = registry->create();
        registry->emplace<engine::ui::UiComponent>(application_gui_entity,
                                                   Rx::make_ptr<ui::ApplicationGui>(RX_SYSTEM_ALLOCATOR, ui_controller));
    }
} // namespace sanity::editor
