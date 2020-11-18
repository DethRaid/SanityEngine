#pragma once
#include "rx/core/rex_string.h"

namespace ui {
    /*!
     * \brief A floating window. ZOriginal use case includes editing worldgen parameters and other global settings
     */
    class Window {
    public:
        bool is_visible = false;

        explicit Window(const Rx::String& name_in);

    	virtual ~Window() = default;

        void draw();

    protected:
        Rx::String name{};
    	
        virtual void draw_contents() = 0;
    };
} // namespace ui
