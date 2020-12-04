#include "SanityEditor.hpp"

#include "entity/Components.hpp"
#include "entt/entity/registry.hpp"
#include "nlohmann/json.hpp"
#include "rx/core/filesystem/file.h"
#include "rx/core/log.h"
#include "sanity_engine.hpp"
#include "ui/ApplicationGui.hpp"
#include "ui/ui_components.hpp"

RX_LOG("SanityEditor", logger);

int main(int argc, char** argv) {
    sanity::engine::initialize_g_engine(R"(E:\Documents\SanityEngine\x64\Debug)");

    sanity::editor::initialize_editor(R"(E:\Documents\SanityEngine\x64\Debug)");

    sanity::editor::g_editor->run_until_quit();

    return 0;
}

namespace sanity::editor {
    SanityEditor::SanityEditor(const char* executable_directory)
        : flycam{engine::g_engine->get_window(), engine::g_engine->get_player(), engine::g_engine->get_global_registry()} {
        create_application_gui();

        auto& renderer = engine::g_engine->get_renderer();
        asset_loader = Rx::make_ptr<engine::AssetLoader>(RX_SYSTEM_ALLOCATOR, &renderer);
    }

    void SanityEditor::run_until_quit() {
        auto* window = engine::g_engine->get_window();

        while(glfwWindowShouldClose(window) == GLFW_FALSE) {
            glfwPollEvents();

            engine::g_engine->do_frame();
        }
    }

    engine::AssetLoader& SanityEditor::get_asset_loader() const { return *asset_loader; }

    ui::EditorUiController& SanityEditor::get_ui_controller() { return ui_controller; }

    AssetRegistry& SanityEditor::get_asset_registry() { return asset_registry; }

    void SanityEditor::create_application_gui() {
        auto registry = engine::g_engine->get_global_registry().lock();

        const auto application_gui_entity = registry->create();
        registry->emplace<engine::ui::UiComponent>(application_gui_entity,
                                                   Rx::make_ptr<ui::ApplicationGui>(RX_SYSTEM_ALLOCATOR, ui_controller));
    }

    void initialize_editor(const char* executable_directory) { g_editor = new SanityEditor{executable_directory}; }
} // namespace sanity::editor
