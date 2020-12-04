#include "content_browser.hpp"

#include "SanityEditor.hpp"
#include "glm/vec2.hpp"
#include "rx/core/filesystem/directory.h"
#include "rx/core/log.h"
#include "rx/core/string.h"
#include "rx/core/utility/move.h"
#include "sanity_engine.hpp"

namespace sanity::editor::ui {
    RX_LOG("ContentBrowser", logger);

    ContentBrowser::ContentBrowser(Rx::String content_directory_in)
        : Window{"Content Browser"}, content_directory{Rx::Utility::move(content_directory_in)}, selected_directory{content_directory} {}

    void ContentBrowser::draw_contents() {
        Rx::Filesystem::Directory dir{selected_directory};

        // Current location we're drawing content items at
        glm::uvec2 cur_item_cursor_location{0, 0};

        dir.each([&](Rx::Filesystem::Directory::Item&& item) {
            if(item.is_directory()) {
                draw_filesystem_item(item, [&] { selected_directory = Rx::String::format("%s/%s", selected_directory, item.name()); });
            }
        });
    }

    void ContentBrowser::draw_filesystem_item(const Rx::Filesystem::Directory::Item& item, const Rx::Function<void()>& on_open) {
        const auto& item_name = item.name();
        if(ImGui::BeginChild(item_name.data())) {
            if(item.is_file()) {
                const auto dot_pos = item_name.find_last_of(".");
                const auto file_extension = item_name.substring(dot_pos + 1);

                if(const auto* icon_handle = file_icons.find(file_extension); icon_handle != nullptr) {
                    ImGui::Image(reinterpret_cast<ImTextureID>(icon_handle->index), {50, 50});

                } else {
                    auto& asset_loader = g_editor->get_asset_loader();
                    const auto& icon_filepath = Rx::String::format("%s/data/textures/%s.png",
                                                                   engine::SanityEngine::executable_directory,
                                                                   file_extension);
                    auto handle = asset_loader.load_image(icon_filepath, [&](const engine::ImageLoadResult& result) {
                        if(result.succeeded) {
                            file_icons.insert(item_name, *result.asset);
                        } else {
                            logger->error("Could not load icon for file type .%s", file_extension);
                        }
                    });
                    icon_handles.push_back(Rx::Utility::move(handle));
                }
            }

            ImGui::Text("%s", item_name.data());
        }
        ImGui::EndChild();
        const auto size = ImGui::GetItemRectSize();
        const auto pos = ImGui::GetCursorPos();

        ImGui::SetCursorPosY(pos.y - size.y);

        bool is_item_selected = false;
        ImGui::Selectable(item.name().data(), &is_item_selected, 0, size);
        if(is_item_selected) {
            on_open();
        }
    }
} // namespace sanity::editor::ui
