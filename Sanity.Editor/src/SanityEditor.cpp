#include "SanityEditor.hpp"

#include <filesystem>

#include "Tracy.hpp"
#include "entity/Components.hpp"
#include "entt/entity/registry.hpp"
#include "nlohmann/json.hpp"
#include "rx/core/concurrency/thread.h"
#include "rx/core/filesystem/file.h"
#include "rx/core/log.h"
#include "sanity_engine.hpp"
#include "serialization/project_serialization.hpp"
#include "ui/ApplicationGui.hpp"
#include "ui/ui_components.hpp"

using namespace sanity::engine;

RX_LOG("SanityEditor", logger);

int main(int argc, char** argv) {
    const std::filesystem::path executable_path{argv[0]};
    const auto executable_directory = executable_path.parent_path();

    initialize_g_engine(executable_directory);

    auto* editor = sanity::editor::initialize_editor(R"(C:\Users\gold1\Documents\SanityEngine\SanityEngine\Sanity.Game\SumerianGame.json)");

    editor->run_until_quit();

    return 0;
}

namespace sanity::editor {
    SanityEditor::SanityEditor(const std::filesystem::path& initial_project_file)
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
    }

    void SanityEditor::load_project(const std::filesystem::path& project_file, const bool should_scan_project_directory) {
        const auto project_file_string = project_file.string();
        const auto file_contents_opt = Rx::Filesystem::read_text_file(project_file_string.c_str());
        if(!file_contents_opt.has_value()) {
            logger->error("Could not load project file %s", project_file);
            return;
        }

        const auto& file_contents = *file_contents_opt;

        try {
            const auto json_contents = nlohmann::json::parse(file_contents.data(), file_contents.data() + file_contents.size());

            json_contents.get_to(project_data);

            const auto enclosing_directory = project_file.parent_path();
            content_directory = enclosing_directory / "content";

            ui_controller.set_content_browser_directory(content_directory);
        }
        catch(const std::exception& ex) {
            logger->error("Could not deserialize project file %s: %s", project_file, ex.what());
            return;
        }

        if(should_scan_project_directory) {
            scan_project_directory_async(content_directory);
        }
    }

    void SanityEditor::scan_project_directory_async(const std::filesystem::path& project_content_directory) {
        Rx::Concurrency::Thread project_dir_scan_thread{"Project dir scanner", [&](const Int32 num) {
                                                            // Recursively iterate over all the files in the project dir, running the
                                                            // importer for any that have changed
                                                        }};
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

    const std::filesystem::path& SanityEditor::get_content_directory() const { return content_directory; }

    void SanityEditor::create_application_gui() {
        auto registry = g_engine->get_global_registry().lock();

        const auto application_gui_entity = registry->create();
        registry->emplace<engine::ui::UiComponent>(application_gui_entity,
                                                   Rx::make_ptr<ui::ApplicationGui>(RX_SYSTEM_ALLOCATOR, ui_controller));
    }

    SanityEditor* initialize_editor(const std::filesystem::path& initial_project_directory) {
        g_editor = new SanityEditor{initial_project_directory};
        return g_editor;
    }
} // namespace sanity::editor
