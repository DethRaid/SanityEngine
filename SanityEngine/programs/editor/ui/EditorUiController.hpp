#pragma once

#include <entt/fwd.hpp>

#include "ApplicationGui.hpp"
#include "rx/core/ptr.h"
#include "windows/WorldgenParamsEditor.hpp"

namespace sanity::editor::ui {
    class EditorUiController {
    public:
        /*!
         * \brief Creates a new instance of the editor UI, adding entities for it to the provided registry
         */
        explicit EditorUiController();

    	void draw();

    	void show_worldgen_params_editor();

    private:
        Rx::Ptr<ApplicationGui> main_gui;

    	WorldgenParamsEditor worldgen_params_editor{};
    	
        void create_application_gui();
    };
} // namespace sanity::editor::ui
