#include "SanityEditor.hpp"

#include "rx/core/abort.h"


#include "rx/core/log.h"

RX_LOG("SanityEditor", logger);

int main(int argc, char** argv) {
    auto editor = sanity::editor::SanityEditor{R"(E:\Documents\SanityEngine\x64\Debug)"};

    editor.run_until_quit();

    return 0;
}

namespace sanity::editor {
    SanityEditor::SanityEditor(const char* executable_directory)
    {
        g_engine = new SanityEngine{executable_directory};
        engine = g_engine;
    	
	    create_editor_ui();
    }

    void SanityEditor::run_until_quit() const {
        auto* window = engine->get_window();

        while(glfwWindowShouldClose(window) == GLFW_FALSE) {
            glfwPollEvents();

            engine->tick();

            glfwSwapBuffers(window);
        }
    }

    void SanityEditor::create_editor_ui() const {
        auto& registry_synchronizer = engine->get_registry();
        auto registry = registry_synchronizer.lock();
    }
} // namespace sanity::editor
