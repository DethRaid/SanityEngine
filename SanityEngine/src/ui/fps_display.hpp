#pragma once

#include "ui_panel.hpp"

class FramerateTracker;

namespace ui {
    class FramerateDisplay : public virtual UiPanel {
    public:
        explicit FramerateDisplay(FramerateTracker& tracker_in);

        ~FramerateDisplay() override = default;

        void draw() override;

    private:
        FramerateTracker* tracker{nullptr};
    };
} // namespace ui
