#include "EditorUiController.hpp"

#include <entt/entity/registry.hpp>

#include "ApplicationGui.hpp"
#include "adapters/rex/rex_wrapper.hpp"
#include "ui/ui_components.hpp"

namespace sanity::editor::ui {
    EditorUiController::EditorUiController() { create_application_gui(); }

    void EditorUiController::draw()
    {
	    main_gui->draw();

    	if(worldgen_params_editor.is_visible)
    	{
            worldgen_params_editor.draw();
    	}
    }

    void EditorUiController::show_worldgen_params_editor()
    { worldgen_params_editor.is_visible = true;
    }

    void EditorUiController::create_application_gui() { main_gui = Rx::make_ptr<ApplicationGui>(RX_SYSTEM_ALLOCATOR, *this); }
} // namespace sanity::editor::ui
