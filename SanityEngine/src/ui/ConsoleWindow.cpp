#include "ConsoleWindow.hpp"

#include "core/types.hpp"
#include "imgui/imgui.h"
#include "rx/console/context.h"
#include "rx/console/variable.h"
#include "rx/core/log.h"

namespace sanity::engine::ui {
    RX_LOG("ConsoleWindow", logger);

    constexpr Uint32 INPUT_BUFFER_SIZE = 2048;

    ConsoleWindow::ConsoleWindow(Rx::Console::Context& console_context_in) : Window{"Console"}, console_context{console_context_in} {
        input_buffer.resize(INPUT_BUFFER_SIZE);
        logger->verbose("Created a ConsoleWindow for input buffer size &d", INPUT_BUFFER_SIZE);
    }

    void ConsoleWindow::draw_contents() {
        input_buffer.resize(INPUT_BUFFER_SIZE);
        ImGui::InputText("Command:", input_buffer.data(), input_buffer.size());
    }
} // namespace sanity::engine::ui
