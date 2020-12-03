#include "SanityEditor.hpp"

#include <rex/include/rx/core/filesystem/file.h>

#include "entity/Components.hpp"
#include "entt/entity/entity.hpp"
#include "entt/entity/registry.hpp"
#include "nlohmann/json.hpp"
#include "rx/core/abort.h"
#include "rx/core/log.h"
#include "sanity_engine.hpp"
#include "serialization/EntitySerialization.hpp"
#include "ui/ApplicationGui.hpp"
#include "ui/ui_components.hpp"
#include "windows/windows_helpers.hpp"

RX_LOG("SanityEditor", logger);

int main(int argc, char** argv) {
    sanity::engine::initialize_g_engine(R"(E:\Documents\SanityEngine\x64\Debug)");

    auto editor = sanity::editor::SanityEditor{R"(E:\Documents\SanityEngine\x64\Debug)"};

    editor.run_until_quit();

    return 0;
}

namespace sanity::editor {
    SanityEditor::SanityEditor(const char* executable_directory)
        : flycam{engine::g_engine->get_window(), engine::g_engine->get_player(), engine::g_engine->get_global_registry()} {
        test_saving_entity();

        create_application_gui();
    }

    void SanityEditor::run_until_quit() {
        auto* window = engine::g_engine->get_window();

        while(glfwWindowShouldClose(window) == GLFW_FALSE) {
            glfwPollEvents();

            engine::g_engine->do_frame();
        }
    }

    void SanityEditor::test_saving_entity() {
        auto registry = engine::g_engine->get_global_registry().lock();

        const auto entity = registry->create();

        auto& transform = registry->emplace<TransformComponent>(entity);

        auto& component_list = registry->emplace<ComponentClassIdList>(entity);
        component_list.class_ids.push_back(__uuidof(TransformComponent));

        const auto& json_entity = serialization::entity_to_json(entity, *registry);

        const auto& json_string = nlohmann::to_string(json_entity);

        Rx::Filesystem::File file{R"(E:\Documents\SanityEngine\x64\Debug\entity.json)", "w"};
        if(!file.is_valid()) {
            logger->error("Could not save entity to file: %s", get_last_windows_error());
            return;
        }

        file.print(json_string.c_str());
    }

    void SanityEditor::create_application_gui() {
        auto registry = engine::g_engine->get_global_registry().lock();

        const auto application_gui_entity = registry->create();
        registry->emplace<engine::ui::UiComponent>(application_gui_entity,
                                                   Rx::make_ptr<ui::ApplicationGui>(RX_SYSTEM_ALLOCATOR, ui_controller));
    }
} // namespace sanity::editor
