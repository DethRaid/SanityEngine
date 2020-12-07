#include "content_browser.hpp"

#include "SanityEditor.hpp"
#include "asset_registry/asset_registry.hpp"
#include "glm/vec2.hpp"
#include "rx/core/filesystem/directory.h"
#include "rx/core/log.h"
#include "rx/core/string.h"
#include "rx/core/utility/move.h"
#include "sanity_engine.hpp"

namespace sanity::editor::ui {
    RX_LOG("ContentBrowser", logger);

    constexpr Uint32 DIRECTORY_ITEM_WIDTH = 200;

    ContentBrowser::ContentBrowser(Rx::String content_directory_in)
        : Window{"Content Browser"}, content_directory{Rx::Utility::move(content_directory_in)}, selected_directory{content_directory} {}

    void ContentBrowser::draw_contents() {
        Rx::Filesystem::Directory dir{selected_directory};

        // Current location we're drawing content items at
        glm::uvec2 cur_item_cursor_location{0, 0};

        if(content_directory != selected_directory) {
            draw_back_button();

        } else {
            ImGui::Text("Content root");
        }

        const auto width = ImGui::GetWindowWidth();
        const auto num_columns = floor(width / DIRECTORY_ITEM_WIDTH);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {2, 2});
        ImGui::Columns(static_cast<Uint32>(num_columns));
        ImGui::Separator();

        dir.each([&](Rx::Filesystem::Directory::Item&& item) {
            if(item.is_directory()) {
                draw_filesystem_item(item, [&](const Rx::Filesystem::Directory::Item& selected_item) {
                    selected_directory = Rx::String::format("%s/%s", selected_directory, selected_item.name());
                });

            } else if(item.is_file()) {
                if(item.name().ends_with(".meta")) {
                	// Skip meta files
                    return;
                }

                draw_filesystem_item(item, [&](const Rx::Filesystem::Directory::Item& selected_item) {
                    const auto filename = Rx::String::format("%s/%s", selected_directory, selected_item.name());
                    g_editor->get_ui_controller().show_editor_for_asset(filename);
                });
            }
        });

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    void ContentBrowser::draw_back_button() {
        if(ImGui::Button("..")) {
            const auto last_slash_pos = selected_directory.find_last_of("/");
            const auto new_selected_directory = selected_directory.substring(0, last_slash_pos);
            selected_directory = new_selected_directory;
        }
    }

    void ContentBrowser::draw_filesystem_item(const Rx::Filesystem::Directory::Item& item,
                                              const Rx::Function<void(const Rx::Filesystem::Directory::Item&)>& on_open) {
        const auto& item_name = item.name();

        auto& asset_registry = g_editor->get_asset_registry();

        const auto file_icon = asset_registry.get_icon_for_file(item_name);
        ImGui::Image(reinterpret_cast<ImTextureID>(file_icon.index), {20, 20});
        ImGui::SameLine();

        if(ImGui::Button(item_name.data(), {DIRECTORY_ITEM_WIDTH - 20, 20})) {
            on_open(item);
        }
    }
} // namespace sanity::editor::ui
