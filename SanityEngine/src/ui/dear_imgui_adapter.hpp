#pragma once

#include <windows.h>
#include <entt/entity/view.hpp>

#include "ui_components.hpp"

/*!
 * \brief Adapter class to hook Dear ImGUI into Sanity Engine. Largely based on the GLFW impl in the Dear ImGUI examples
 */
class DearImguiAdapter {
public:
    explicit DearImguiAdapter(HWND hwnd);

    ~DearImguiAdapter();

    void draw_ui(const entt::basic_view<entt::entity, entt::exclude_t<>, ui::UiComponent>& view);
};
