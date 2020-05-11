#pragma once

namespace ui {
    class UiPanel {
    public:
        UiPanel() = default;

        UiPanel(const UiPanel& other) = default;
        UiPanel& operator=(const UiPanel& other) = default;

        UiPanel(UiPanel&& old) noexcept = default;
        UiPanel& operator=(UiPanel&& old) noexcept = default;

        virtual ~UiPanel() = default;

        virtual void draw() = 0;
    };
}
