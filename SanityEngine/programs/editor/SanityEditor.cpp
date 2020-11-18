#include "SanityEditor.hpp"

#include "rx/core/global.h"

static Rx::GlobalGroup s_sanity_editor_globals{"Sanity.Editor"};

RX_CONSOLE_IVAR(window_width, "Window.Width", "Width of the SanityEditor window", 0, 8196, 640);
RX_CONSOLE_IVAR(window_height, "Window.Height", window_height, "Width of the SanityEditor window", 0, 8196, 480);

int main(int argc, char** argv) {
    auto editor = sanity::editor::SanityEditor{};

    editor.run_until_quit();

    return 0;
}

namespace sanity::editor {
    SanityEditor::SanityEditor() {
        // Make a window for the editor to display in
        main_window = Rx::make_ptr<ui::ApplicationWindow>(RX_SYSTEM_ALLOCATOR, *window_width, *window_height);
        engine = Rx::make_ptr<SanityEngine>(RX_SYSTEM_ALLOCATOR,
                                            R"(E:\Documents\SanityEngine\x64\Debug)",
                                            main_window->get_window_handle(),
                                            glm::ivec2{window_width->get(), window_height->get()});

        main_window->set_window_user_pointer(engine->get_input_manager());

        create_editor_ui();
    }

    void SanityEditor::run_until_quit() {
        while(!main_window->should_close()) {
            const auto is_visible = main_window->is_visible();
            engine->Tick(is_visible);
        }
    }

    void SanityEditor::create_editor_ui() {
        auto& registry_synchronizer = engine->get_registry();
        auto registry = registry_synchronizer.lock();
    }
} // namespace sanity::editor
