#pragma once

#include <entt/fwd.hpp>

namespace sanity::editor::ui {
    class EditorUiController {
    public:
        /*!
         * \brief Creates a new instance of the editor UI, adding entities for it to the provided registry
         */
        explicit EditorUiController(entt::registry& registry);

    	void show_worldgen_params_editor();

    private:
        entt::entity application_gui;
    	
        void create_application_menu(entt::registry& registry);
    };
} // namespace sanity::editor::ui
