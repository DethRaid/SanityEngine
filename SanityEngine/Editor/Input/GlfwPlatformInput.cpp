#include "GlfwPlatformInput.hpp"

#include "GLFW/glfw3.h"

namespace Sanity::Editor::Input {
    GlfwPlatformInput::GlfwPlatformInput(GLFWwindow* window_in) : window{window_in} {}

    bool GlfwPlatformInput::is_key_down(const int key) { return glfwGetKey(window, key) == GLFW_PRESS; }

    Double2 GlfwPlatformInput::get_mouse_location() {
        Double2 location;
        glfwGetCursorPos(window, &location.x, &location.y);
        return location;
    }
} // namespace Sanity::Editor::Input
