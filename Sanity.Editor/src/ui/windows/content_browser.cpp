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

    constexpr Uint32 DIRECTORY_ITEM_WIDTH = 512;

    ContentBrowser::ContentBrowser() : Window{"Content Browser"} { register_builtin_file_context_menus(); }

    void ContentBrowser::set_content_directory(const std::filesystem::path& content_directory_in) {
        content_directory = content_directory_in;
        selected_directory = *content_directory;
        is_visible = true;
    }

    void ContentBrowser::add_ignored_file_extension(const std::filesystem::path& extension) { file_extensions_to_ignore.insert(extension); }

    void ContentBrowser::remove_ignored_file_extension(const std::filesystem::path& extension) {
        file_extensions_to_ignore.erase(extension);
    }

    void ContentBrowser::draw_contents() {
        if(!content_directory.has_value()) {
            logger->verbose("No content directory set, aborting");
            return;
        }

        Rx::Vector<std::filesystem::path> directory_names;
        Rx::Vector<std::filesystem::path> file_names;

        const auto directory_to_draw = engine::SanityEngine::executable_directory / *content_directory / selected_directory;

        for(const auto& item : std::filesystem::directory_iterator{directory_to_draw}) {
            if(item.is_directory()) {
                directory_names.push_back(item.path().stem());

            } else if(item.is_regular_file() && !should_ignore_file(item.path())) {
                file_names.push_back(item.path().filename());
            }
        }

        const auto width = ImGui::GetWindowWidth();
        const auto num_columns = static_cast<Uint32>(ceil(width / DIRECTORY_ITEM_WIDTH));

        const auto num_items = directory_names.size() + file_names.size();
        const auto num_rows = static_cast<Uint32>(floor(num_items / num_columns));

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {2, 2});
        ImGui::Columns(num_columns);
        ImGui::Separator();

        if(*content_directory != selected_directory) {
            draw_back_button();

        } else {
            ImGui::Text("Content root");
        }

        auto cur_row = 1u; // Start at 1 because we count `..`

        directory_names.each_fwd([&](const std::filesystem::path& directory) {
            draw_directory(directory, [&](const std::filesystem::path& selected_item) { selected_directory /= selected_item; });

            cur_row++;
            if(cur_row == num_rows) {
                ImGui::NextColumn();
                cur_row = 0;
            }
        });

        file_names.each_fwd([&](const std::filesystem::path& file) {
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

    void show_skybox_menu(const std::filesystem::path& file) {
        if(ImGui::Button("Use as skybox")) {
            engine::g_engine->get_world().set_skybox(file);
        }
    }

    void ContentBrowser::register_builtin_file_context_menus() {
        file_extension_context_menus.insert(".hdr", [](const std::filesystem::path& file) { show_skybox_menu(file); });
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
        if(ImGui::Button(directory_string.c_str(), {0, 20})) {
            on_open(directory);
        }
    }

    void ContentBrowser::draw_file(const std::filesystem::path& file, const Rx::Function<void(const std::filesystem::path&)>& on_open) {
        const auto file_name = file.string();

        ImGui::PushID(file_name.c_str());

        auto& asset_registry = g_editor->get_asset_registry();

        const auto file_icon = asset_registry.get_icon_for_extension(file.extension());

        ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<Uint64>(file_icon.index)), {20, 20});
        ImGui::SameLine();

        if(ImGui::Button(file_name.c_str(), {0, 20})) {
            on_open(file);
        }

        if(const auto* draw_context_menu = file_extension_context_menus.find(file.extension()); draw_context_menu != nullptr) {
            if(ImGui::BeginPopupContextItem("Context menu", ImGuiMouseButton_Right)) {
                (*draw_context_menu)(selected_directory / file);
                ImGui::EndPopup();
            }
        }

        ImGui::PopID();
    }

    bool ContentBrowser::should_ignore_file(const std::filesystem::path& file) const {
        return file_extensions_to_ignore.find(file.extension()) != nullptr;
    }
} // namespace sanity::editor::ui
