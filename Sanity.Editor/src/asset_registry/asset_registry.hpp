#pragma once

#include "asset_registry/asset_metadata_json_conversion.hpp"
#include "nlohmann/json.hpp"
#include "renderer/handles.hpp"
#include "rx/core/filesystem/file.h"
#include "rx/core/map.h"

namespace Rx {
    struct String;
}

namespace sanity::editor {
    class AssetRegistry {
    public:
        template <typename AssetImportSettingsType>
        static AssetMetadata<AssetImportSettingsType> get_meta_for_asset(const Rx::String& asset_path);

        explicit AssetRegistry();

        engine::renderer::TextureHandle get_icon_for_file(const Rx::String& file_path);

    private:
        Rx::Map<Rx::String, engine::renderer::TextureHandle> known_file_icons;

        engine::renderer::TextureHandle directory_icon;

        void load_directory_icon();
    };

    template <typename AssetImportSettingsType>
    AssetMetadata<AssetImportSettingsType> AssetRegistry::get_meta_for_asset(const Rx::String& asset_path) {
        AssetMetadata<MeshImportSettings> meta{};

        const auto meta_path = Rx::String::format("%s.meta", asset_path);
        const auto meta_maybe = Rx::Filesystem::read_text_file(meta_path);
        if(meta_maybe) {
            const auto meta_json = nlohmann::json::parse(meta_maybe->data(), meta_maybe->data() + meta_maybe->size());
            meta_json.get_to(meta);
        }

        return meta;
    }
} // namespace sanity::editor
