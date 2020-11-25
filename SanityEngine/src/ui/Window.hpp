#pragma once

#include "ui_panel.hpp"
#include "rx/core/string.h"

namespace ui {
    /*!
     * \brief A floating window
     */
    class Window : public UiPanel {
    public:
        bool is_visible = false;

        explicit Window(const Rx::String& name_in);

    	~Window() override = default;

        void draw() override;

    protected:
        Rx::String name{};
    	
        virtual void draw_contents() = 0;
    };
} // namespace ui
