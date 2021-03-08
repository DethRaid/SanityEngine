#include "asset_registry.hpp"

#include <renderer/rhi/d3d12_private_data.hpp>


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
        engine::renderer::TextureFormat format;
        const auto* pixels = engine::load_texture("data/textures/icons/directory.png", width, height, format);
        if(pixels == nullptr) {
            logger->error("Could not load directory icon at path data/textures/icons/directory.png");
            directory_icon = renderer.get_pink_texture();
            return;
        }
        
        directory_icon = renderer.create_texture(engine::renderer::TextureCreateInfo{.name = "Directory icon",
                                                                                 .usage = engine::renderer::TextureUsage::SampledTexture,
                                                                                 .format = format,
                                                                                 .width = width,
                                                                                 .height = height},
                                               pixels);
    }

    engine::renderer::TextureHandle load_icon_for_extension(const std::filesystem::path& extension) {
        const auto path = std::filesystem::path{"data/textures/icons"} / engine::append_extension(extension, ".png");

        auto& renderer = engine::g_engine->get_renderer();

        Uint32 width;
        Uint32 height;
        engine::renderer::TextureFormat format;
        const auto* pixels = engine::load_texture(path, width, height, format);
        if(pixels == nullptr) {
            logger->error("Could not load icon at path '%s'", path);
            return renderer.get_pink_texture();
        }

        const auto extension_string = extension.string();
        const auto create_info = engine::renderer::TextureCreateInfo{.name = Rx::String::format(".%s icon", extension_string.c_str()),
                                                                   .usage = engine::renderer::TextureUsage::SampledTexture,
                                                                   .format = format,
                                                                   .width = width,
                                                                   .height = height};
        return renderer.create_texture(create_info, pixels);
    }
} // namespace sanity::editor
