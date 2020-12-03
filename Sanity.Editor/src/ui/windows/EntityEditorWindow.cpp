#include "EntityEditorWindow.hpp"

namespace sanity::editor::ui {
    EntityEditorWindow::EntityEditorWindow(entt::entity& entity_in, entt::registry& registry_in)
        : Window{"Entity Editor"}, entity{entity_in}, registry{registry_in}
    {
	    
    }

    void EntityEditorWindow::draw_contents() {
    	
    }
} // namespace sanity::editor::ui
