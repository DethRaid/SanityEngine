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

        if(content_directory != selected_directory) {
            draw_back_button();
        	
        } else {
            ImGui::Text("Content root");
        }

        dir.each([&](Rx::Filesystem::Directory::Item&& item) {
            if(item.is_directory()) {
                draw_filesystem_item(item, [&](const Rx::Filesystem::Directory::Item& selected_item) {
                    selected_directory = Rx::String::format("%s/%s", selected_directory, selected_item.name());
                });
            }
        });
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

        if(ImGui::Button(item_name.data(), {100, 100})) {
            on_open(item);
        }
    }
} // namespace sanity::editor::ui
