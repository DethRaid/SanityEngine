#pragma once

#include "rx/core/ptr.h"
#include "ui_panel.hpp"

namespace ui {
    struct UiComponent {
        Rx::Ptr<UiPanel> panel;

        UiComponent() = default;

        explicit UiComponent(Rx::Ptr<UiPanel> panel_in);
    };
} // namespace ui
