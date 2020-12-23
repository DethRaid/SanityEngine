#pragma once

#include "core/async/synchronized_resource.hpp"
#include "entt/entity/registry.hpp"
#include "ui/ui_components.hpp"
#include "windows/content_browser.hpp"

namespace std {
	namespace filesystem {
		class path;
	}
}

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

        void create_and_edit_new_entity() const;

        void set_content_browser_directory(const std::filesystem::path& content_directory) const;

#pragma region Editors
        void show_editor_for_asset(const std::filesystem::path& asset_path) const;

        void open_mesh_import_settings(const std::filesystem::path& mesh_path) const;
#pragma endregion

    private:
        WorldgenParamsEditor* worldgen_params_editor{nullptr};

        ContentBrowser* content_browser{nullptr};

        void create_worldgen_params_editor(SynchronizedResourceAccessor<entt::registry, Rx::Concurrency::Mutex>& registry);

        void create_content_browser(entt::registry& registry, const std::filesystem::path& content_directory = "");

        template <typename WindowType, typename... Args>
        WindowType* create_window_entity(entt::registry& registry, Args&&... args) const;
    };

    template <typename WindowType, typename... Args>
    WindowType* EditorUiController::create_window_entity(entt::registry& registry, Args&&... args) const {
        const auto window_entity = registry.create();

        auto window_ptr = Rx::make_ptr<WindowType>(RX_SYSTEM_ALLOCATOR, Rx::Utility::forward<Args>(args)...);
        auto& ui_component = registry.emplace<engine::ui::UiComponent>(window_entity, Rx::Utility::move(window_ptr));
        auto* window = static_cast<WindowType*>(ui_component.panel.get());

        return window;
    }
} // namespace sanity::editor::ui
