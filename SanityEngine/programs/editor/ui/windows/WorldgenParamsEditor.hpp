#pragma once

#include "ui/Window.hpp"

namespace sanity::editor::ui {
    class WorldgenParamsEditor final : public engine::ui::Window {
    public:
        WorldgenParamsEditor();

    protected:
        void draw_contents() override;
    };
} // namespace sanity::editor::ui
