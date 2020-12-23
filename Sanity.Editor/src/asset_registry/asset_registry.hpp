#pragma once

#include <filesystem>

#include "asset_registry/asset_metadata_json_conversion.hpp"
#include "nlohmann/json.hpp"
#include "renderer/handles.hpp"
#include "rx/core/filesystem/file.h"
#include "rx/core/log.h"
#include "rx/core/map.h"

namespace sanity::editor {
    RX_LOG("AssetRegistry", ar_logger);

    class AssetRegistry {
    public:
        template <typename ImportSettingsType>
        static AssetMetadata<ImportSettingsType> get_meta_for_asset(const std::filesystem::path& asset_path);

        template <typename ImportSettingsType>
        static void save_meta_for_asset(const std::filesystem::path& asset_path, const AssetMetadata<ImportSettingsType>& metadata);

        explicit AssetRegistry();

        [[nodiscard]] engine::renderer::TextureHandle get_icon_for_extension(const std::filesystem::path& extension);

        [[nodiscard]] engine::renderer::TextureHandle get_directory_icon() const;

    private:
        Rx::Map<std::filesystem::path, engine::renderer::TextureHandle> known_file_icons;

        engine::renderer::TextureHandle directory_icon;

        void load_directory_icon();
    };

    template <typename ImportSettingsType>
    AssetMetadata<ImportSettingsType> AssetRegistry::get_meta_for_asset(const std::filesystem::path& asset_path) {
        AssetMetadata<SceneImportSettings> meta{};

        const auto meta_path = asset_path.string().append(".meta");
        const auto meta_maybe = Rx::Filesystem::read_text_file(meta_path.c_str());
        if(meta_maybe) {
            const auto meta_json = nlohmann::json::parse(meta_maybe->data(), meta_maybe->data() + meta_maybe->size());
            meta_json.get_to(meta);
        }

        return meta;
    }

    template <typename ImportSettingsType>
    void AssetRegistry::save_meta_for_asset(const std::filesystem::path& asset_path,
                                                const AssetMetadata<ImportSettingsType>& metadata) {
        const nlohmann::json json = metadata;
        const auto json_string = nlohmann::to_string(json);

        const auto meta_path = asset_path.string().append(".meta");
        auto file = Rx::Filesystem::File{meta_path.c_str(), "w"};
        if(!file.is_valid()) {
            ar_logger->error("Could not save metadata for asset '%s'", asset_path);
            return;
        }

        file.print(json_string.c_str());
    }
} // namespace sanity::editor
