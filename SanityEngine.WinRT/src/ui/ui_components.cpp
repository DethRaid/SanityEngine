#include "ui_components.hpp"

namespace ui {
    UiComponent::UiComponent(Rx::Ptr<UiPanel> panel_in) : panel{Rx::Utility::move(panel_in)} {}
} // namespace ui
