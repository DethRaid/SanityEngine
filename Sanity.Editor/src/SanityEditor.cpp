#include "SanityEditor.hpp"

#include "Tracy.hpp"
#include "entity/Components.hpp"
#include "entt/entity/registry.hpp"
#include "nlohmann/json.hpp"
#include "rx/core/filesystem/file.h"
#include "rx/core/log.h"
#include "sanity_engine.hpp"
#include "serialization/project_serialization.hpp"
#include "ui/ApplicationGui.hpp"
#include "ui/ui_components.hpp"

using namespace sanity::engine;

RX_LOG("SanityEditor", logger);

int main(int argc, char** argv) {
    initialize_g_engine(R"(E:\Documents\SanityEngine\x64\Debug)");

    auto* editor = sanity::editor::initialize_editor(R"(E:\Documents\SanityEngine\Sanity.Game\SumerianGame.json)");

    editor->run_until_quit();

    return 0;
}

namespace sanity::editor {
    SanityEditor::SanityEditor(const Rx::String& initial_project_file)
        : flycam{g_engine->get_window(), g_engine->get_player(), g_engine->get_global_registry()} {
        load_project(initial_project_file);
        create_application_gui();

        auto& renderer = g_engine->get_renderer();
        asset_loader = Rx::make_ptr<AssetLoader>(RX_SYSTEM_ALLOCATOR, &renderer);

        g_engine->register_tick_function([&](const Float32 delta_time) {
            auto* window = g_engine->get_window();
            // Only tick if we have focus
            if(glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE) {
                flycam.update_player_transform(delta_time);
            }
        });

        // Stupid model viewing hack
        const auto player_entity = g_engine->get_player();
        auto registry = g_engine->get_global_registry().lock();
        auto& player_transform = registry->get<TransformComponent>(player_entity);
        player_transform.transform.location = {0, 0, 10};
    }

    void SanityEditor::load_project(const Rx::String& project_file) {
        const auto file_contents_opt = Rx::Filesystem::read_text_file(project_file);
        if(!file_contents_opt.has_value()) {
            logger->error("Could not load project file %s", project_file);
            return;
        }

        const auto& file_contents = *file_contents_opt;

        try {
            const auto json_contents = nlohmann::json::parse(file_contents.data(), file_contents.data() + file_contents.size());

            json_contents.get_to(project_data);

            auto slashpos = project_file.find_last_of("/");
            if(slashpos == Rx::String::k_npos) {
                slashpos = project_file.find_last_of(R"(\)");
            }

            const auto enclosing_directory = project_file.substring(0, slashpos);
            content_directory = Rx::String::format("%s/content", enclosing_directory);

            ui_controller.set_content_browser_directory(content_directory);
        }
        catch(const std::exception& ex) {
            logger->error("Could not deserialize project file %s: %s", project_file, ex.what());
        }
    }

    void SanityEditor::run_until_quit() {
        auto* window = g_engine->get_window();

        while(glfwWindowShouldClose(window) == GLFW_FALSE) {
            glfwPollEvents();

            g_engine->tick();
        }
    }

    AssetLoader& SanityEditor::get_asset_loader() const { return *asset_loader; }

    ui::EditorUiController& SanityEditor::get_ui_controller() { return ui_controller; }

    AssetRegistry& SanityEditor::get_asset_registry() { return asset_registry; }

    const Rx::String& SanityEditor::get_content_directory() const { return content_directory; }

    void SanityEditor::create_application_gui() {
        auto registry = g_engine->get_global_registry().lock();

        const auto application_gui_entity = registry->create();
        registry->emplace<engine::ui::UiComponent>(application_gui_entity,
                                                   Rx::make_ptr<ui::ApplicationGui>(RX_SYSTEM_ALLOCATOR, ui_controller));
    }

    SanityEditor* initialize_editor(const Rx::String& initial_project_directory) {
        g_editor = new SanityEditor{initial_project_directory};
        return g_editor;
    }
} // namespace sanity::editor
