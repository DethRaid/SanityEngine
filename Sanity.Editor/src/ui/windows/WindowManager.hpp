#pragma once

namespace Rx {
    struct String;
}

namespace sanity {
    namespace engine {
        namespace ui {
            class Window;
        }
    } // namespace engine

    namespace editor::ui {
        constexpr const char* WORLDGEN_PARAMS_EDITOR_NAME = "Worldgen Params Editor";

        void show_window(const Rx::String& window_name);

        void hide_window(const Rx::String& window_name);

        [[nodisacrd]] engine::ui::Window* get_window(const Rx::String& window_name);

    } // namespace editor::ui
} // namespace sanity
