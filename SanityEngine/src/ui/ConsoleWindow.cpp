#include "ConsoleWindow.hpp"

#include <cstdio>

#include <inttypes.h>

#include "GLFW/glfw3.h"
#include "core/types.hpp"
#include "imgui/imgui.h"
#include "rx/console/context.h"
#include "rx/console/variable.h"
#include "rx/core/log.h"

namespace sanity::engine::ui {
    RX_LOG("ConsoleWindow", logger);

    constexpr Uint32 INPUT_BUFFER_SIZE = 2048;

    void draw_console_line(const Rx::String& line);

    ConsoleWindow::ConsoleWindow(Rx::Console::Context& console_context_in)
        : Window{"Console", ImGuiWindowFlags_NoCollapse}, console_context{console_context_in} {
        input_buffer.resize(INPUT_BUFFER_SIZE);
        logger->verbose("Created a ConsoleWindow for input buffer size &d", INPUT_BUFFER_SIZE);
    }

    Rx::Vector<Rx::String> ConsoleWindow::format_lines(const Rx::Vector<Rx::String>& console_lines) {
        Rx::Vector<Rx::String> formatted_lines{};

        formatted_lines.reserve(console_lines.size());

        console_lines.each_fwd([&](const Rx::String& line) { formatted_lines.push_back(Rx::String::format("> %s\n", line)); });

        return formatted_lines;
    }

    void ConsoleWindow::draw_contents() {
        input_buffer.resize(INPUT_BUFFER_SIZE);

        const auto& console_lines = console_context.lines();
        const auto console_output = format_lines(console_lines);

        const auto height = ImGui::GetWindowHeight();
        const auto console_output_height = height - 30;

        // if(ImGui::BeginChild("#ConsoleOutput", ImVec2{-1, console_output_height})) {
        //     console_output.each_fwd([](const Rx::String& line) { draw_console_line(line); });
        // }
        // ImGui::EndChild();

    	console_output.each_fwd([](const Rx::String& line) { draw_console_line(line); });
    	
        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint("", "Command", input_buffer.data(), input_buffer.size());
        ImGui::PopItemWidth();

        if(ImGui::IsKeyPressed(GLFW_KEY_ENTER)) {
            console_context.execute(input_buffer);
            input_buffer.clear();
        }
    }

    ImVec4 hex_to_color(const Uint32 hex) {
        ImVec4 color;

        color.x = (hex & 0xff) / 255.0f;
        color.y = ((hex >> 8) & 0xff) / 255.0f;
        color.z = ((hex >> 16) & 0xff) / 255.0f;
        color.w = ((hex >> 24) & 0xff) / 255.0f;

        return color;
    }

    void draw_console_line(const Rx::String& line) {
        ImVec4 color = {1.0f, 1.0f, 1.0f, 1.0f}; // White
        ImGui::PushStyleColor(ImGuiCol_Text, color);

        const char* beg = line.data();

    	// End pointer. Advance it until we find a ^. If we never find one, that's great, just print the whole string! If
    	// we do, then print everything until the ^
        const char* end = line.data();
        do {
            end = strchr(beg, '^');        	
            if(end == nullptr) {
            	// No more color specifiers in the string. Print the rest of the string as-is
                ImGui::TextUnformatted(beg, beg + strlen(beg));
                break;
            }

            ImGui::TextUnformatted(beg, end);
            ImGui::SameLine();
        	
            int n = 1; // Skip '^'
            switch(end[n]) {
                case '^':
                    break; // Do nothing

                case 'r':
                    color = {1.0f, 0.0f, 0.0f, 1.0f};
                    n++;
                    break;

                case 'g':
                    color = {0.0f, 1.0f, 0.0f, 1.0f};
                    n++;
                    break;

                case 'b':
                    color = {0.0f, 0.0f, 1.0f, 1.0f};
                    n++;
                    break;

                case 'c':
                    color = {0.0f, 1.0f, 1.0f, 1.0f};
                    n++;
                    break;
            	
                case 'y':
                    color = {1.0f, 1.0f, 0.0f, 1.0f};
                    n++;
                    break;
            	
                case 'm':
                    color = {1.0f, 0.0f, 1.0f, 1.0f};
                    n++;
                    break;
            	
                case 'k':
                    color = {0.0f, 0.0f, 0.0f, 1.0f};
                    n++;
                    break;
            	
                case 'w':
                    color = {1.0f, 1.0f, 1.0f, 1.0f};
                    n++;
                    break;
            	
                case '[':
                    if(const char* term = strchr(end + n, ']')) {
                        Uint32 hex = 0;
                        sscanf(end + n, "[%" PRIx32 "]", &hex);
                        color = hex_to_color(hex);
                        n = term - end + n + 1; // skip the hex code
                    }
            		break;
            }

            // Swap colors.
            ImGui::PopStyleColor();
            ImGui::PushStyleColor(ImGuiCol_Text, color);

            end += n; // advance
            beg = end;
        } while(*end);

        ImGui::PopStyleColor();
    }
} // namespace sanity::engine::ui
