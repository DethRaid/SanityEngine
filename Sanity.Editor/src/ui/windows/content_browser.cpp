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
    constexpr Uint32 DIRECTORY_ITEM_HEIGHT = 30;

    ContentBrowser::ContentBrowser(Rx::String content_directory_in)
        : Window{"Content Browser"},
          content_directory{Rx::Utility::move(content_directory_in)},
          selected_directory{content_directory},
          file_extensions_to_ignore{Rx::Array{".meta", ".blend1", ".blend2", ".blend2"}} {}

    void ContentBrowser::set_content_directory(const Rx::String& content_directory_in) {
        content_directory = content_directory_in;
        selected_directory = content_directory;
    }

    void ContentBrowser::add_ignored_file_extension(const Rx::String& extension) { file_extensions_to_ignore.insert(extension); }

    void ContentBrowser::remove_ignored_file_extension(const Rx::String& extension) { file_extensions_to_ignore.erase(extension); }

    void ContentBrowser::draw_contents() {
        Rx::Filesystem::Directory dir{selected_directory};

        if(content_directory != selected_directory) {
            draw_back_button();

        } else {
            ImGui::Text("Content root");
        }

        const auto width = ImGui::GetWindowWidth();
        const auto num_columns = static_cast<Uint32>(floor(width / DIRECTORY_ITEM_WIDTH));

        const auto height = ImGui::GetWindowHeight();
        const auto num_rows = static_cast<Uint32>(floor(height / DIRECTORY_ITEM_HEIGHT));

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {2, 2});
        ImGui::Columns(num_columns);
        ImGui::Separator();

        auto cur_row = 0u;

        dir.each([&](Rx::Filesystem::Directory::Item&& item) {
            if(item.is_directory()) {
                draw_filesystem_item(item, [&](const Rx::Filesystem::Directory::Item& selected_item) {
                    selected_directory = Rx::String::format("%s/%s", selected_directory, selected_item.name());
                });

            } else if(item.is_file()) {
                if(should_ignore_file(item)) {
                    return;
                }

                draw_filesystem_item(item, [&](const Rx::Filesystem::Directory::Item& selected_item) {
                    const auto filename = Rx::String::format("%s/%s", selected_directory, selected_item.name());
                    g_editor->get_ui_controller().show_editor_for_asset(filename);
                });
            }

            cur_row++;
            if(cur_row == num_rows) {
                ImGui::NextColumn();
                cur_row = 0;
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

        const auto file_icon = item.is_directory() ? asset_registry.get_directory_icon() : asset_registry.get_file_icon(item_name);

        ImGui::Image(reinterpret_cast<ImTextureID>(file_icon.index), {20, 20});
        ImGui::SameLine();

        if(ImGui::Button(item_name.data(), {DIRECTORY_ITEM_WIDTH - 20, 20})) {
            on_open(item);
        }
    }

    bool ContentBrowser::should_ignore_file(const Rx::Filesystem::Directory::Item& file) {
        auto should_ignore = false;
        file_extensions_to_ignore.each([&](const Rx::String& extension) {
            if(file.name().ends_with(extension)) {
                should_ignore = true;
                return RX_ITERATION_STOP;
            }

            return RX_ITERATION_CONTINUE;
        });
        return should_ignore;
    }
} // namespace sanity::editor::ui
