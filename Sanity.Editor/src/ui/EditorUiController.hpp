#pragma once

#include "entt/entity/registry.hpp"

namespace sanity::editor::ui {
    class EditorUiController {
    public:
        /*!
         * \brief Creates a new instance of the editor UI, adding entities for it to the provided registry
         */
        explicit EditorUiController();

    	void show_worldgen_params_editor();

    private:
    	entt::entity worldgen_params_editor{};

        void create_worldgen_params_editor();
    };
} // namespace sanity::editor::ui
