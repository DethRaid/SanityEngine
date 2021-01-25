#pragma once

#include <filesystem>

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
        explicit ContentBrowser();

        void set_content_directory(const std::filesystem::path& content_directory_in);

        void add_ignored_file_extension(const std::filesystem::path& extension);

        void remove_ignored_file_extension(const std::filesystem::path& extension);

    protected:
        void draw_contents() override;

    private:
        void register_builtin_file_context_menus();
    	
        void draw_directory(const std::filesystem::path& directory, const Rx::Function<void(const std::filesystem::path&)>& on_open);

        void draw_file(const std::filesystem::path& file, const Rx::Function<void(const std::filesystem::path&)>& on_open);
        
        Rx::Optional<std::filesystem::path> content_directory;

        std::filesystem::path selected_directory;

        Rx::Map<std::filesystem::path, engine::renderer::TextureHandle> file_icons;

        Rx::Vector<engine::ImageLoadResultHandle> icon_handles;

        Rx::Set<std::filesystem::path> file_extensions_to_ignore{Rx::Array{".meta", ".bin", ".blend1", ".blend2", ".blend2"}};

        Rx::Map<std::filesystem::path, Rx::Function<void(const std::filesystem::path&)>> file_extension_context_menus;

        void draw_back_button();

        [[nodiscard]] bool should_ignore_file(const std::filesystem::path& file) const;
    };
} // namespace sanity::editor::ui
