#include "Window.hpp"

#include "imgui/imgui.h"

namespace ui {
    Window::Window(const Rx::String& name_in) : name{Rx::Utility::move(name_in)} {}

    void Window::draw() {
        if(ImGui::Begin(name.data(), &is_visible)) {
            draw_contents();
        }
        ImGui::End();
    }
} // namespace ui