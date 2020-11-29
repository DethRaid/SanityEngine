#pragma once

#include "imgui/imgui.h"
#include "ui_panel.hpp"
#include "rx/core/string.h"

namespace sanity::engine::ui {
    /*!
     * \brief A floating window
     */
    class Window : public UiPanel {
    public:
        bool is_visible = false;

        explicit Window(const Rx::String& name_in, ImGuiWindowFlags flags_in = 0);

    	~Window() override = default;

        void draw() override;

    protected:
        Rx::String name{};

        ImGuiWindowFlags flags;
    	
        virtual void draw_contents() = 0;
    };
} // namespace ui
