#pragma once

#include "core/async/synchronized_resource.hpp"
#include "entt/entity/registry.hpp"

namespace sanity {
    namespace engine {
        namespace ui {
            class Window;
        }
    } // namespace engine
} // namespace sanity

namespace sanity::editor::ui {
    class WorldgenParamsEditor;
    class EntityEditorWindow;

    class EditorUiController {
    public:
        /*!
         * \brief Creates a new instance of the editor UI, adding entities for it to the provided registry
         */
        explicit EditorUiController();

        void show_worldgen_params_editor() const;

        EntityEditorWindow* show_edit_entity_window(entt::entity& entity, entt::registry& registry) const;
    	
        void create_and_edit_new_entity();

    private:
        WorldgenParamsEditor* worldgen_params_editor{nullptr};

        void create_worldgen_params_editor(SynchronizedResourceAccessor<entt::registry, Rx::Concurrency::Mutex>& registry);
    };
} // namespace sanity::editor::ui
