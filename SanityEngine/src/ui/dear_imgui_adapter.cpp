#include "dear_imgui_adapter.hpp"

#define GLFW_EXPOSE_NATIVE_WIN32

#include <glfw/glfw3native.h>
#include <imgui/imgui.h>

static GLFWmousebuttonfun prev_mouse_button_callback;
static GLFWscrollfun prev_scroll_callback;
static GLFWkeyfun prev_key_callback;
static GLFWcharfun prev_char_callback;
static bool g_MouseJustPressed[5] = {false, false, false, false, false};

static const char* get_clipboard_text(void* user_data) { return glfwGetClipboardString(static_cast<GLFWwindow*>(user_data)); }

static void set_clipboard_text(void* user_data, const char* text) { glfwSetClipboardString(static_cast<GLFWwindow*>(user_data), text); }

void mouse_button_callback(GLFWwindow* window, const int button, const int action, const int mods) {
    if(prev_mouse_button_callback != nullptr) {
        prev_mouse_button_callback(window, button, action, mods);
    }

    if(action == GLFW_PRESS && button >= 0 && button < IM_ARRAYSIZE(g_MouseJustPressed)) {
        g_MouseJustPressed[button] = true;
    }
}

void scroll_callback(GLFWwindow* window, const double xoffset, const double yoffset) {
    if(prev_scroll_callback != nullptr) {
        prev_scroll_callback(window, xoffset, yoffset);
    }

    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheelH += static_cast<float>(xoffset);
    io.MouseWheel += static_cast<float>(yoffset);
}

void key_callback(GLFWwindow* window, const int key, const int scancode, const int action, const int mods) {
    if(prev_key_callback != nullptr) {
        prev_key_callback(window, key, scancode, action, mods);
    }

    auto& io = ImGui::GetIO();
    if(action == GLFW_PRESS) {
        io.KeysDown[key] = true;
    }
    if(action == GLFW_RELEASE) {
        io.KeysDown[key] = false;
    }

    // Modifiers are not reliable across systems
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
    io.KeySuper = false;
}

void char_callback(GLFWwindow* window, unsigned int c) {
    if(prev_char_callback != nullptr) {
        prev_char_callback(window, c);
    }

    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(c);
}

void DearImguiAdapter::update_mouse_pos_and_buttons() const {
    // Update buttons
    ImGuiIO& io = ImGui::GetIO();
    for(int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++) {
        // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter
        // than 1 frame.
        io.MouseDown[i] = g_MouseJustPressed[i] || glfwGetMouseButton(window, i) != 0;
        g_MouseJustPressed[i] = false;
    }

    // Update mouse position
    const ImVec2 mouse_pos_backup = io.MousePos;
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

    const bool focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
    if(focused) {
        if(io.WantSetMousePos) {
            glfwSetCursorPos(window, static_cast<double>(mouse_pos_backup.x), static_cast<double>(mouse_pos_backup.y));
        } else {
            double mouse_x, mouse_y;
            glfwGetCursorPos(window, &mouse_x, &mouse_y);
            io.MousePos = ImVec2(static_cast<float>(mouse_x), static_cast<float>(mouse_y));
        }
    }
}

void DearImguiAdapter::update_mouse_cursor() const {
    ImGuiIO& io = ImGui::GetIO();
    if((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
        return;
    }

    const auto imgui_cursor = ImGui::GetMouseCursor();
    if(imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor) {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    } else {
        // Show OS mouse cursor
        // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
        glfwSetCursor(window, mouse_cursors[imgui_cursor] ? mouse_cursors[imgui_cursor] : mouse_cursors[ImGuiMouseCursor_Arrow]);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

DearImguiAdapter::DearImguiAdapter(GLFWwindow* window_in) : window{window_in} {
    ImGui::CreateContext();

    auto& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // Enable mouse
    io.BackendPlatformName = "Sanity Engine";
    io.ImeWindowHandle = glfwGetWin32Window(window);

    io.SetClipboardTextFn = set_clipboard_text;
    io.GetClipboardTextFn = get_clipboard_text;
    io.ClipboardUserData = window;

    mouse_cursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    mouse_cursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    mouse_cursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    mouse_cursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    mouse_cursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    mouse_cursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    mouse_cursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    mouse_cursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    mouse_cursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);

    prev_mouse_button_callback = glfwSetMouseButtonCallback(window, mouse_button_callback);
    prev_scroll_callback = glfwSetScrollCallback(window, scroll_callback);
    prev_key_callback = glfwSetKeyCallback(window, key_callback);
    prev_char_callback = glfwSetCharCallback(window, char_callback);
}

DearImguiAdapter::~DearImguiAdapter() { ImGui::DestroyContext(); }

void DearImguiAdapter::draw_ui(const entt::basic_view<entt::entity, entt::exclude_t<>, ui::UiComponent>& view) {
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(
        io.Fonts->IsBuilt() &&
        "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(window, &w, &h);
    glfwGetFramebufferSize(window, &display_w, &display_h);
    io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));
    if(w > 0 && h > 0) {
        io.DisplayFramebufferScale = ImVec2(static_cast<float>(display_w) / w, static_cast<float>(display_h) / h);
    }

    // Setup time step
    const double current_time = glfwGetTime();
    io.DeltaTime = last_start_time > 0.0 ? static_cast<float>(current_time - last_start_time) : static_cast<float>(1.0f / 60.0f);
    last_start_time = current_time;

    update_mouse_pos_and_buttons();
    update_mouse_cursor();

    ImGui::NewFrame();

    view.each([](const ui::UiComponent& component) { component.panel->draw(); });

    ImGui::EndFrame();
    // ImGui::Render();
}
