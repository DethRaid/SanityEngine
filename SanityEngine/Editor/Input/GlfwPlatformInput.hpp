#pragma once
#include "input/PlatformInput.hpp"

struct GLFWwindow;

namespace Sanity::Editor::Input {
    class GlfwPlatformInput : public PlatformInput {
    public:
        explicit GlfwPlatformInput(GLFWwindow* window_in);

        [[nodiscard]] bool is_key_down(int key) override;
    	
        [[nodiscard]] Double2 get_mouse_location() override;
    	
    private:
        GLFWwindow* window{nullptr};
    };
} // namespace Sanity::Editor::Input
