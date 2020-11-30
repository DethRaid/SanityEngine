#include "WindowManager.hpp"

#include "entt/entity/entity.hpp"
#include "rx/core/global.h"
#include "rx/core/map.h"
#include "sanity_engine.hpp"
#include "WorldgenParamsEditor.hpp"
#include "ui/Window.hpp"

namespace sanity::editor::ui {
    Rx::Global<Rx::Map<Rx::String, entt::entity>> window_map{"SanityEditor", "WindowMap"};

    static void create_window(const Rx::String& window_name);

    void show_window(const Rx::String& window_name) {
        auto registry = engine::g_engine->get_registry().lock();

        if(const auto* window_ptr = window_map->find(window_name); window_ptr != nullptr) {
            auto* ui_comp = registry->try_get<engine::ui::UiComponent>(*window_ptr);
            auto* window = static_cast<engine::ui::Window*>(ui_comp->panel.get());
            window->is_visible = true;
        	
        } else {
            create_window(window_name);
        }
    }

	void create_window(const Rx::String& window_name)
    { entt::entity window_entity;
        if(window_name == WORLDGEN_PARAMS_EDITOR_NAME)
        {
            window_entity = create_window_entity<WorldgenParamsEditor>();
        }
    }

} // namespace sanity::editor::ui
