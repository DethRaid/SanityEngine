#include "asset_registry.hpp"

#include "loading/image_loading.hpp"
#include "rx/core/log.h"
#include "sanity_engine.hpp"

namespace sanity::editor {
    RX_LOG("AssetRegistry", logger);

    engine::renderer::TextureHandle load_icon_for_extension(const Rx::String& extension);

    AssetRegistry::AssetRegistry() { load_directory_icon(); }

    engine::renderer::TextureHandle AssetRegistry::get_icon_for_file(const Rx::String& file_path) {
        if(const auto dot_pos = file_path.find_last_of("."); dot_pos != Rx::String::k_npos) {
            const auto extension = file_path.substring(dot_pos + 1);
            if(const auto* icon_handle_ptr = known_file_icons.find(extension); icon_handle_ptr != nullptr) {
                return *icon_handle_ptr;

            } else {
                const auto icon_handle = load_icon_for_extension(extension);
                known_file_icons.insert(extension, icon_handle);
                return icon_handle;
            }
        } else {
            return directory_icon;
        }
    }

    void AssetRegistry::load_directory_icon() {
        auto& renderer = engine::g_engine->get_renderer();

        Uint32 width;
        Uint32 height;
        Rx::Vector<Uint8> pixels;
        if(!engine::load_image("data/textures/icons/directory.png", width, height, pixels)) {
            logger->error("Could not load directory icon at path data/textures/icons/directory.png");
            directory_icon = renderer.get_pink_texture();
            return;
        }

        auto cmds = renderer.get_render_backend().create_command_list();

        directory_icon = renderer.create_image(engine::renderer::ImageCreateInfo{.name = "Directory icon",
                                                                                 .usage = engine::renderer::ImageUsage::SampledImage,
                                                                                 .format = engine::renderer::ImageFormat::Rgba8,
                                                                                 .width = width,
                                                                                 .height = height},
                                               pixels.data(),
                                               cmds);

        renderer.get_render_backend().submit_command_list(Rx::Utility::move(cmds));
    }

    engine::renderer::TextureHandle load_icon_for_extension(const Rx::String& extension) {
        const auto path = Rx::String::format("data/textures/icons/%s.png", extension);

        auto& renderer = engine::g_engine->get_renderer();

        Uint32 width;
        Uint32 height;
        Rx::Vector<Uint8> pixels;
        if(!engine::load_image(path, width, height, pixels)) {
            logger->error("Could not load icon at path '%s'", path);
            return renderer.get_pink_texture();
        }

        const auto create_info = engine::renderer::ImageCreateInfo{.name = Rx::String::format(".%s icon", extension),
                                                                   .usage = engine::renderer::ImageUsage::SampledImage,
                                                                   .format = engine::renderer::ImageFormat::Rgba8,
                                                                   .width = width,
                                                                   .height = height};
        auto cmds = renderer.get_render_backend().create_command_list();
        const auto icon_handle = renderer.create_image(create_info, pixels.data(), cmds);
        renderer.get_render_backend().submit_command_list(Rx::Utility::move(cmds));

        return icon_handle;
    }
} // namespace sanity::editor
