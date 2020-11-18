#include "SanityEditor.hpp"

int main(int argc, char** argv) {
    auto editor = sanity::editor::SanityEditor{};

    editor.run_until_quit();

    return 0;
}

namespace sanity::editor {
    SanityEditor::SanityEditor() { create_editor_ui(); }

    void SanityEditor::run_until_quit() {
        auto* window = engine->get_window();

        while(glfwWindowShouldClose(window) == GLFW_FALSE) {
            engine->tick();
        }
    }

    void SanityEditor::create_editor_ui() {
        auto& registry_synchronizer = engine->get_registry();
        auto registry = registry_synchronizer.lock();
    }
} // namespace sanity::editor
