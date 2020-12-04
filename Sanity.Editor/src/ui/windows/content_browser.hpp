#pragma once

#include "loading/asset_loader.hpp"
#include "renderer/handles.hpp"
#include "rx/core/filesystem/directory.h"
#include "rx/core/map.h"
#include "ui/Window.hpp"

namespace Rx {
    struct String;
} // namespace Rx

namespace sanity::editor::ui {
    class ContentBrowser : public engine::ui::Window {
    public:
        explicit ContentBrowser(Rx::String content_directory_in);

    protected:
        void draw_contents() override;

    private:
        Rx::String content_directory;

        Rx::String selected_directory;

        Rx::Map<Rx::String, engine::renderer::TextureHandle> file_icons;

    	Rx::Vector<engine::ImageLoadResultHandle> icon_handles;

        void draw_filesystem_item(const Rx::Filesystem::Directory::Item& item, const Rx::Function<void()>& on_open);
    };
} // namespace sanity::editor::ui
