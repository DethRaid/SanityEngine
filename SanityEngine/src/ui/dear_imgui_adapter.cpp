#include "dear_imgui_adapter.hpp"

#include <imgui/imgui.h>

DearImguiAdapter::DearImguiAdapter(HWND hwnd) {
    ImGui::CreateContext();

    auto& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // Enable mouse
    io.BackendPlatformName = "Sanity Engine";
    io.ImeWindowHandle = hwnd;
}

DearImguiAdapter::~DearImguiAdapter() {
    ImGui::DestroyContext();
}

void DearImguiAdapter::draw_ui(const entt::basic_view<entt::entity, entt::exclude_t<>, ui::UiComponent>& view) {

    ImGui::NewFrame();

    view.each([](const ui::UiComponent& component) { component.panel->draw(); });

    ImGui::EndFrame();
    // ImGui::Render();
}

