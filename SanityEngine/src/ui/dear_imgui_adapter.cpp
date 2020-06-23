#include "dear_imgui_adapter.hpp"

#define GLFW_EXPOSE_NATIVE_WIN32

#include <GLFW/glfw3native.h>
#include <Tracy.hpp>
#include <imgui/imgui.h>

#include "renderer/renderer.hpp"
#include "rhi/render_device.hpp"
#include "rhi/resources.hpp"

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

void scroll_callback(GLFWwindow* window, const double x_offset, const double y_offset) {
    if(prev_scroll_callback != nullptr) {
        prev_scroll_callback(window, x_offset, y_offset);
    }

    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheelH += static_cast<Float32>(x_offset);
    io.MouseWheel += static_cast<Float32>(y_offset);
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

void char_callback(GLFWwindow* window, const unsigned int c) {
    if(prev_char_callback != nullptr) {
        prev_char_callback(window, c);
    }

    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(c);
}

DearImguiAdapter::DearImguiAdapter(GLFWwindow* window_in, renderer::Renderer& renderer) : window{window_in} {
    ZoneScoped;
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

    initialize_style();

    create_font_texture(renderer);
}

DearImguiAdapter::~DearImguiAdapter() { ImGui::DestroyContext(); }

void DearImguiAdapter::draw_ui(const entt::basic_view<entt::entity, entt::exclude_t<>, ui::UiComponent>& view) {
    ZoneScoped;

    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(
        io.Fonts->IsBuilt() &&
        "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(window, &w, &h);
    glfwGetFramebufferSize(window, &display_w, &display_h);
    io.DisplaySize = ImVec2(static_cast<Float32>(w), static_cast<Float32>(h));
    if(w > 0 && h > 0) {
        io.DisplayFramebufferScale = ImVec2(static_cast<Float32>(display_w) / static_cast<Float32>(w),
                                            static_cast<Float32>(display_h) / static_cast<Float32>(h));
    }

    // Setup time step
    const double current_time = glfwGetTime();
    io.DeltaTime = last_start_time > 0.0 ? static_cast<Float32>(current_time - last_start_time) : static_cast<Float32>(1.0f / 60.0f);
    last_start_time = current_time;

    update_mouse_pos_and_buttons();
    update_mouse_cursor();

    ImGui::NewFrame();

    view.each([](const ui::UiComponent& component) { component.panel->draw(); });

    ImGui::Render();
}

void DearImguiAdapter::initialize_style() {
    // from https://github.com/ocornut/imgui/issues/707#issuecomment-468798935
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    /// 0 = FLAT APPEARENCE
    /// 1 = MORE "3D" LOOK
    int is_3d = 1;

    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
    colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
    colors[ImGuiCol_Separator] = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

    style.PopupRounding = 3;

    style.WindowPadding = ImVec2(4, 4);
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(6, 2);

    style.ScrollbarSize = 18;

    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = static_cast<Float32>(is_3d);

    style.WindowRounding = 3;
    style.ChildRounding = 3;
    style.FrameRounding = 3;
    style.ScrollbarRounding = 2;
    style.GrabRounding = 3;

#ifdef IMGUI_HAS_DOCK
    style.TabBorderSize = is3D;
    style.TabRounding = 3;

    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);

    if(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
#endif
}

void DearImguiAdapter::create_font_texture(renderer::Renderer& renderer) {
    const auto commands = renderer.get_render_device().create_command_list();

    auto& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    const auto create_info = renderer::ImageCreateInfo{.name = "Dear ImGUI Font Atlas",
                                                  .usage = renderer::ImageUsage::SampledImage,
                                                  .format = renderer::ImageFormat::Rgba8,
                                                  .width = static_cast<Uint32>(width),
                                                  .height = static_cast<Uint32>(height)};

    font_atlas = renderer.create_image(create_info, pixels, commands);

    renderer.get_render_device().submit_command_list(commands);

    const uint64_t imgui_tex_id = font_atlas.index;

    io.Fonts->TexID = reinterpret_cast<ImTextureID>(imgui_tex_id);
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
            io.MousePos = ImVec2(static_cast<Float32>(mouse_x), static_cast<Float32>(mouse_y));
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
