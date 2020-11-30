#include "Window.hpp"

#include "imgui/imgui.h"

namespace sanity::engine::ui {
    Window::Window(const Rx::String& name_in, const ImGuiWindowFlags flags_in) : name{Rx::Utility::move(name_in)}, flags{flags_in} {}

    void Window::draw() {
        if(is_visible) {
            if(ImGui::Begin(name.data(), &is_visible, flags)) {
                draw_contents();
            }
            ImGui::End();
        }
    }
} // namespace sanity::engine::ui