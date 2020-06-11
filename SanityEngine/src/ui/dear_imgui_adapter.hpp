#pragma once

#include <GLFW/glfw3.h>
#include <entt/entity/view.hpp>
#include <imgui/imgui.h>

#include "ui_components.hpp"
#include "../renderer/handles.hpp"
#include "../renderer/material_data_buffer.hpp"

namespace renderer {
    class Renderer;
}

/*!
 * \brief Adapter class to hook Dear ImGUI into Sanity Engine. Largely based on the GLFW impl in the Dear ImGUI examples
 */
class DearImguiAdapter {
public:
    explicit DearImguiAdapter(GLFWwindow* window_in, renderer::Renderer& renderer);

    DearImguiAdapter(const DearImguiAdapter& other) = delete;
    DearImguiAdapter& operator=(const DearImguiAdapter& other) = delete;

    DearImguiAdapter(DearImguiAdapter&& old) noexcept = delete;
    DearImguiAdapter& operator=(DearImguiAdapter&& old) noexcept = delete;

    ~DearImguiAdapter();

    void draw_ui(const entt::basic_view<entt::entity, entt::exclude_t<>, ui::UiComponent>& view);

private:
    GLFWwindow* window;
    std::array<GLFWcursor*, ImGuiMouseCursor_COUNT> mouse_cursors{};
    double last_start_time{0};

    renderer::TextureHandle font_atlas;
    renderer::StandardMaterialHandle font_material;

    void initialize_style();

    void create_font_texture(renderer::Renderer& renderer);

    void update_mouse_pos_and_buttons() const;

    void update_mouse_cursor() const;
};
