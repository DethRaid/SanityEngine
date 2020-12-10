#pragma once

#include "asset_registry/asset_metadata_json_conversion.hpp"
#include "nlohmann/json.hpp"
#include "renderer/handles.hpp"
#include "rx/core/filesystem/file.h"
#include "rx/core/log.h"
#include "rx/core/map.h"

namespace Rx {
    struct String;
}

namespace sanity::editor {
    RX_LOG("AssetRegistry", ar_logger);

    class AssetRegistry {
    public:
        template <typename ImportSettingsType>
        static AssetMetadata<ImportSettingsType> get_meta_for_asset(const Rx::String& asset_path);

        template <typename ImportSettingsType>
        static void save_metadata_for_asset(const Rx::String& asset_path, const AssetMetadata<ImportSettingsType>& metadata);

        explicit AssetRegistry();

        [[nodiscard]] engine::renderer::TextureHandle get_file_icon(const Rx::String& file_path);

        [[nodiscard]] engine::renderer::TextureHandle get_directory_icon() const;

      private:
        Rx::Map<Rx::String, engine::renderer::TextureHandle> known_file_icons;

        engine::renderer::TextureHandle directory_icon;

        void load_directory_icon();
    };

    template <typename ImportSettingsType>
    AssetMetadata<ImportSettingsType> AssetRegistry::get_meta_for_asset(const Rx::String& asset_path) {
        AssetMetadata<SceneImportSettings> meta{};

        const auto meta_path = Rx::String::format("%s.meta", asset_path);
        const auto meta_maybe = Rx::Filesystem::read_text_file(meta_path);
        if(meta_maybe) {
            const auto meta_json = nlohmann::json::parse(meta_maybe->data(), meta_maybe->data() + meta_maybe->size());
            meta_json.get_to(meta);
        }

        return meta;
    }

    template <typename ImportSettingsType>
    void AssetRegistry::save_metadata_for_asset(const Rx::String& asset_path, const AssetMetadata<ImportSettingsType>& metadata) {
        const nlohmann::json json = metadata;
        const auto json_string = nlohmann::to_string(json);

        const auto meta_path = Rx::String::format("%s.meta", asset_path);
        auto file = Rx::Filesystem::File{meta_path, "w"};
        if(!file.is_valid()) {
            ar_logger->error("Could not save metadata for asset '%s'", asset_path);
            return;
        }

        file.print(json_string.c_str());
    }
} // namespace sanity::editor
