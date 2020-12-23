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

    ContentBrowser::ContentBrowser(std::filesystem::path content_directory_in)
        : Window{"Content Browser"},
          content_directory{Rx::Utility::move(content_directory_in)},
          selected_directory{content_directory},
          file_extensions_to_ignore{Rx::Array{".meta", ".blend1", ".blend2", ".blend2"}} {}

    void ContentBrowser::set_content_directory(const std::filesystem::path& content_directory_in) {
        content_directory = content_directory_in;
        selected_directory = content_directory;
    }

    void ContentBrowser::add_ignored_file_extension(const std::filesystem::path& extension) { file_extensions_to_ignore.insert(extension); }

    void ContentBrowser::remove_ignored_file_extension(const std::filesystem::path& extension) {
        file_extensions_to_ignore.erase(extension);
    }

    void ContentBrowser::draw_contents() {
        Rx::Vector<std::filesystem::path> directories;
        Rx::Vector<std::filesystem::path> files;

        for(const auto& item : std::filesystem::directory_iterator{selected_directory}) {
            if(item.is_directory()) {
                directories.push_back(item);

            } else if(item.is_regular_file()) {
                files.push_back(item);
            }
        }

        if(content_directory != selected_directory) {
            draw_back_button();

        } else {
            ImGui::Text("Content root");
        }

        const auto width = ImGui::GetWindowWidth();
        const auto num_columns = static_cast<Uint32>(ceil(width / DIRECTORY_ITEM_WIDTH));

        const auto num_items = directories.size() + files.size();
        const auto num_rows = static_cast<Uint32>(ceil(num_items / num_columns));

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {2, 2});
        ImGui::Columns(num_columns);
        ImGui::Separator();

        auto cur_row = 0u;

        directories.each_fwd([&](const std::filesystem::path& directory) {
            draw_directory(directory, [&](const std::filesystem::path& selected_item) { selected_directory /= selected_item; });

            cur_row++;
            if(cur_row == num_rows) {
                ImGui::NextColumn();
                cur_row = 0;
            }
        });

        files.each_fwd([&](const std::filesystem::path& file) {
            draw_file(file, [&](const std::filesystem::path& selected_item) {
                const auto filename = selected_directory / selected_item;
                g_editor->get_ui_controller().show_editor_for_asset(filename);
            });

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
        draw_directory("..", [&](const std::filesystem::path&) { selected_directory = selected_directory.parent_path(); });
    }

    void ContentBrowser::draw_directory(const std::filesystem::path& directory,
                                        const Rx::Function<void(const std::filesystem::path&)>& on_open) {
        auto& asset_registry = g_editor->get_asset_registry();

        const auto file_icon = asset_registry.get_directory_icon();

        ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<Uint64>(file_icon.index)), {20, 20});
        ImGui::SameLine();

        const auto directory_string = directory.string();
        if(ImGui::Button(directory_string.c_str(), {DIRECTORY_ITEM_WIDTH - 20, 20})) {
            on_open(directory);
        }
    }

    void ContentBrowser::draw_file(const std::filesystem::path& file, const Rx::Function<void(const std::filesystem::path&)>& on_open) {
        auto& asset_registry = g_editor->get_asset_registry();

        const auto file_icon = asset_registry.get_icon_for_extension(file.extension());

        ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<Uint64>(file_icon.index)), {20, 20});
        ImGui::SameLine();

        const auto file_name = file.string();
        if(ImGui::Button(file_name.c_str(), {DIRECTORY_ITEM_WIDTH - 20, 20})) {
            on_open(file);
        }
    }

    bool ContentBrowser::should_ignore_file(const std::filesystem::path& file) {
        auto should_ignore = false;
        file_extensions_to_ignore.each([&](const std::filesystem::path& extension) {
            if(file.extension() == extension) {
                should_ignore = true;
                return RX_ITERATION_STOP;
            }

            return RX_ITERATION_CONTINUE;
        });

        return should_ignore;
    }
} // namespace sanity::editor::ui
