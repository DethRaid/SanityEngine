#pragma once

#include <memory>

#include "ui_panel.hpp"

namespace ui {
    struct UiComponent {
        std::unique_ptr<UiPanel> panel;

        UiComponent() = default;

        explicit UiComponent(std::unique_ptr<UiPanel> panel_in);
    };
} // namespace ui
