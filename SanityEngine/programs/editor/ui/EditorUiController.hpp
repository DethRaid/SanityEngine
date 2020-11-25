#pragma once

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
    	WorldgenParamsEditor worldgen_params_editor{};
    };
} // namespace sanity::editor::ui
