#include "ui_components.hpp"

namespace ui {
    UiComponent::UiComponent(std::unique_ptr<UiPanel> panel_in) : panel{std::move(panel_in)} {}
} // namespace ui
