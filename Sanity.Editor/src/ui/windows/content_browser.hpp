#pragma once

#include "loading/asset_loader.hpp"
#include "renderer/handles.hpp"
#include "rx/core/filesystem/directory.h"
#include "rx/core/map.h"
#include "rx/core/set.h"
#include "ui/Window.hpp"

namespace Rx {
    struct String;
} // namespace Rx

namespace sanity::editor::ui {
    class ContentBrowser : public engine::ui::Window {
    public:
        explicit ContentBrowser(Rx::String content_directory_in);

        void set_content_directory(const Rx::String& content_directory_in);

        void add_ignored_file_extension(const Rx::String& extension);

        void remove_ignored_file_extension(const Rx::String& extension);

    protected:
        void draw_contents() override;

    private:
        Rx::String content_directory;

        Rx::String selected_directory;

        Rx::Map<Rx::String, engine::renderer::TextureHandle> file_icons;

        Rx::Vector<engine::ImageLoadResultHandle> icon_handles;

        Rx::Set<Rx::String> file_extensions_to_ignore;

        void draw_back_button();

        void draw_filesystem_item(const Rx::Filesystem::Directory::Item& item,
                                  const Rx::Function<void(const Rx::Filesystem::Directory::Item&)>& on_open);

        bool should_ignore_file(const Rx::Filesystem::Directory::Item& file);
    };
} // namespace sanity::editor::ui
