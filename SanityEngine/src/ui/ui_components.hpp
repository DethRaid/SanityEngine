#pragma once

#include "rx/core/ptr.h"
#include "ui_panel.hpp"

namespace sanity::engine::ui {
    struct UiComponent {
        RX_MARK_NO_COPY(UiComponent);

        Rx::Ptr<UiPanel> panel;

        UiComponent() = default;

        UiComponent(UiComponent&& old) noexcept = default;

        UiComponent& operator=(UiComponent&& old) noexcept = default;

        ~UiComponent() = default;

        explicit UiComponent(Rx::Ptr<UiPanel> panel_in);
    };
} // namespace sanity::engine::ui
