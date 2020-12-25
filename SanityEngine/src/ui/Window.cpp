#include "ui/Window.hpp"

#include "imgui/imgui.h"
#include "rx/core/utility/move.h"

namespace sanity::engine::ui {
    Window::Window(const Rx::String& name_in, const ImGuiWindowFlags flags_in) : UiPanel{Rx::Utility::move(name_in)}, flags{flags_in} {}

    void Window::draw() {
        if(is_visible) {
            if(ImGui::Begin(name.data(), &is_visible, flags)) {
                draw_contents();
            }
            ImGui::End();

            if(!is_visible) {
                destroy_self();
            }
        }
    }

    void Window::destroy_self() {
        // TODO
    }
} // namespace sanity::engine::ui