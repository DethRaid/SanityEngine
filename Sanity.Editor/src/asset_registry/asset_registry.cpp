#include "asset_registry.hpp"

#include "core/fs/path_ops.hpp"
#include "loading/image_loading.hpp"
#include "rx/core/log.h"
#include "sanity_engine.hpp"

namespace sanity::editor {
    RX_LOG("AssetRegistry", logger);

    engine::renderer::TextureHandle load_icon_for_extension(const std::filesystem::path& extension);

    AssetRegistry::AssetRegistry() { load_directory_icon(); }

    engine::renderer::TextureHandle AssetRegistry::get_icon_for_extension(const std::filesystem::path& extension) {
        if(const auto* icon_handle_ptr = known_file_icons.find(extension); icon_handle_ptr != nullptr) {
            return *icon_handle_ptr;

        } else {
            const auto icon_handle = load_icon_for_extension(extension);
            known_file_icons.insert(extension, icon_handle);
            return icon_handle;
        }
    }

    engine::renderer::TextureHandle AssetRegistry::get_directory_icon() const { return directory_icon; }

    void AssetRegistry::load_directory_icon() {
        auto& renderer = engine::g_engine->get_renderer();

        Uint32 width;
        Uint32 height;
        engine::renderer::ImageFormat format;
        const auto* pixels = engine::load_image("data/textures/icons/directory.png", width, height, format);
        if(pixels == nullptr) {
            logger->error("Could not load directory icon at path data/textures/icons/directory.png");
            directory_icon = renderer.get_pink_texture();
            return;
        }

        auto cmds = renderer.get_render_backend().create_command_list();

        directory_icon = renderer.create_image(engine::renderer::ImageCreateInfo{.name = "Directory icon",
                                                                                 .usage = engine::renderer::ImageUsage::SampledImage,
                                                                                 .format = format,
                                                                                 .width = width,
                                                                                 .height = height},
                                               pixels,
                                               cmds.Get());

        renderer.get_render_backend().submit_command_list(Rx::Utility::move(cmds));
    }

    engine::renderer::TextureHandle load_icon_for_extension(const std::filesystem::path& extension) {
        const auto path = std::filesystem::path{"data/textures/icons"} / engine::append_extension(extension, ".png");

        auto& renderer = engine::g_engine->get_renderer();

        Uint32 width;
        Uint32 height;
        engine::renderer::ImageFormat format;
        const auto* pixels = engine::load_image(path, width, height, format);
        if(pixels == nullptr) {
            logger->error("Could not load icon at path '%s'", path);
            return renderer.get_pink_texture();
        }

        const auto extension_string = extension.string();
        const auto create_info = engine::renderer::ImageCreateInfo{.name = Rx::String::format(".%s icon", extension_string.c_str()),
                                                                   .usage = engine::renderer::ImageUsage::SampledImage,
                                                                   .format = format,
                                                                   .width = width,
                                                                   .height = height};
        auto cmds = renderer.get_render_backend().create_command_list();
        const auto icon_handle = renderer.create_image(create_info, pixels, cmds.Get());
        renderer.get_render_backend().submit_command_list(Rx::Utility::move(cmds));

        return icon_handle;
    }
} // namespace sanity::editor
