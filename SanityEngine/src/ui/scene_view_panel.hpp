#pragma once

#include "ui_panel.hpp"

namespace ui {
    class SceneViewPanel : public UiPanel {
    public:
        SceneViewPanel(const Renderer& renderer;

        // Inherited via UiPanel
        virtual void draw() override;
    };
} // namespace ui
