#pragma once

#include "ui_panel.hpp"

class FramerateTracker;

namespace sanity::engine::ui {
    class FramerateDisplay : public UiPanel {
    public:
        explicit FramerateDisplay(FramerateTracker& tracker_in);

        ~FramerateDisplay() override = default;

        void draw() override;

    private:
        FramerateTracker* tracker{nullptr};
    };
} // namespace ui
