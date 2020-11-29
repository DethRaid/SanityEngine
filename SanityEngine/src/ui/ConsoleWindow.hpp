#pragma once

#include "ui/Window.hpp"

namespace Rx::Console {
    struct Context;
}

namespace sanity::engine::ui {
    class ConsoleWindow : public Window {
    public:
        explicit ConsoleWindow(Rx::Console::Context& console_context_in);

        ~ConsoleWindow() override = default;

    protected:
        Rx::Console::Context& console_context;

        Rx::String input_buffer;

        void draw_contents() override;
    };
} // namespace sanity::engine::ui
