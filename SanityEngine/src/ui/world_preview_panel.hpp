#pragma once

#include "imgui/imgui.h"
#include "renderer/renderer.hpp"
#include "ui_panel.hpp"

namespace sanity::engine::ui {
    class WorldPreviewPane : public UiPanel {
    public:
        explicit WorldPreviewPane(const renderer::Renderer& renderer_in);

        // Inherited via UiPanel
        virtual void draw() override;

    private:
        const renderer::Renderer* renderer = nullptr;

        ImTextureID renderer_output_texture;
    };
} // namespace ui
