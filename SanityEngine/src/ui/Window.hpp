#pragma once

#include "imgui/imgui.h"
#include "rx/core/string.h"
#include "ui_panel.hpp"

namespace sanity::engine::ui {
    /*!
     * \brief A floating window
     */
    class Window : public UiPanel {
    public:
        bool is_visible = false;

        explicit Window(const Rx::String& name_in = "Window", ImGuiWindowFlags flags_in = 0);

        ~Window() override = default;

        void draw() override;

    protected:
        ImGuiWindowFlags flags;

        virtual void draw_contents() = 0;

        /*!
         * \brief Destroys this window, along with the entity that owns it and all of the entity's components
         */
        void destroy_self();
    };
} // namespace sanity::engine::ui
