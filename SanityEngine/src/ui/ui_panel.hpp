#pragma once

#include "rx/core/string.h"

namespace sanity::engine::ui {
    class UiPanel {
    public:
        Rx::String name;

        explicit UiPanel(const Rx::String& name_in = "UI Panel");

        UiPanel(const UiPanel& other) = default;
        UiPanel& operator=(const UiPanel& other) = default;

        UiPanel(UiPanel&& old) noexcept = default;
        UiPanel& operator=(UiPanel&& old) noexcept = default;

        virtual ~UiPanel() = default;

        virtual void draw() = 0;
    };

    inline UiPanel::UiPanel(const Rx::String& name_in) : name{name_in} {}
} // namespace sanity::engine::ui
