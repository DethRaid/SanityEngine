#include "GlfwPlatformInput.hpp"

#include "GLFW/glfw3.h"

namespace Sanity::Editor::Input {
    int ToGlfwKey(InputKey key);

    GlfwPlatformInput::GlfwPlatformInput(GLFWwindow* window_in) : window{window_in} {}

    bool GlfwPlatformInput::is_key_down(const InputKey key) const {
        const auto glfw_key = ToGlfwKey(key);
        return glfwGetKey(window, glfw_key) == GLFW_PRESS;
    }

    Double2 GlfwPlatformInput::get_mouse_location() const {
        Double2 location;
        glfwGetCursorPos(window, &location.x, &location.y);
        return location;
    }

    int ToGlfwKey(const InputKey key) {
        switch(key) {
            case InputKey::W:
                return GLFW_KEY_W;

            case InputKey::A:
                return GLFW_KEY_A;

            case InputKey::S:
                return GLFW_KEY_S;

            case InputKey::D:
                return GLFW_KEY_D;

            case InputKey::Q:
                return GLFW_KEY_Q;

            case InputKey::E:
                return GLFW_KEY_E;

            case InputKey::SPACE:
                return GLFW_KEY_SPACE;

            case InputKey::SHIFT:
                return GLFW_KEY_LEFT_SHIFT;

            default:
                return GLFW_KEY_UNKNOWN;
        }
    }
} // namespace Sanity::Editor::Input
