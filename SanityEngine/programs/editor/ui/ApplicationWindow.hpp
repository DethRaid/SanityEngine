#define GLFW_EXPOSE_NATIVE_WIN32

#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include "Tracy.hpp"
#include "core/types.hpp"
#include "input/input_manager.hpp"
#include "rx/core/abort.h"
#include "rx/core/log.h"

namespace sanity::editor::ui {
    class ApplicationWindow {
    public:
        ApplicationWindow(Int32 width, Int32 height);

        ~ApplicationWindow();

        [[nodiscard]] HWND get_window_handle() const;

        void set_window_user_pointer(void* user_pointer) const;

        [[nodiscard]] bool should_close() const;

        [[nodiscard]] bool is_visible() const;

    private:
        GLFWwindow* window;
    };

    RX_LOG("ApplicationWindow", logger);

    static void error_callback(const int error, const char* description) { logger->error("%s (GLFW error %d}", description, error); }

    static void key_func(GLFWwindow* window, const int key, int /* scancode */, const int action, const int mods) {
        auto* input_manager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

        input_manager->on_key(key, action, mods);
    }

    inline ApplicationWindow::ApplicationWindow(Int32 width, Int32 height) {
        ZoneScoped;

        {
            ZoneScopedN("glfwInit");
            if(!glfwInit()) {
                Rx::abort("Could not initialize GLFW");
            }
        }

        glfwSetErrorCallback(error_callback);

        {
            ZoneScopedN("glfwCreateWindow");
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            window = glfwCreateWindow(1000, 480, "Sanity Engine", nullptr, nullptr);
            if(window == nullptr) {
                Rx::abort("Could not create GLFW window");
            }
        }

        logger->info("Created window");

        glfwSetKeyCallback(window, key_func);
    }

    inline ApplicationWindow::~ApplicationWindow() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    inline HWND ApplicationWindow::get_window_handle() const { return glfwGetWin32Window(window); }

    inline void ApplicationWindow::set_window_user_pointer(void* user_pointer) const { glfwSetWindowUserPointer(window, user_pointer); }

    inline bool ApplicationWindow::should_close() const { return glfwWindowShouldClose(window) == GLFW_TRUE; }

    inline bool ApplicationWindow::is_visible() const { return glfwGetWindowAttrib(window, GLFW_VISIBLE) == GLFW_TRUE; }
} // namespace sanity::editor::ui
