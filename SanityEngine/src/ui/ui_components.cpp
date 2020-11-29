#include "ui_components.hpp"

namespace sanity::engine::ui {
    UiComponent::UiComponent(Rx::Ptr<UiPanel> panel_in) : panel{Rx::Utility::move(panel_in)} {}
} // namespace ui
