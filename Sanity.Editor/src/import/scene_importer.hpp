#pragma once

#include <loading/asset_loader.hpp>

#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include "renderer/handles.hpp"
#include "rx/core/map.h"
#include "rx/core/optional.h"
#include "rx/core/vector.h"
#include "tiny_gltf.h"

namespace sanity {
    namespace engine {
        namespace renderer {
            class Renderer;
        }
    } // namespace engine
} // namespace sanity

namespace Rx {
    struct String;
}

namespace sanity::editor::import {
    class SceneImporter {
    public:
        explicit SceneImporter(engine::renderer::Renderer& renderer_in);

        [[nodiscard]] Rx::Optional<entt::entity> import_gltf_scene(const Rx::String& scene_path, entt::registry& registry);

    private:
        tinygltf::TinyGLTF importer;

        engine::renderer::Renderer* renderer;

        Rx::Map<Rx::String, engine::renderer::TextureHandle> loaded_textures;

        [[nodiscard]] Rx::Vector<engine::renderer::StandardMaterialHandle> import_all_materials(const tinygltf::Model& scene,
                                                                                                const ComPtr<ID3D12GraphicsCommandList4>& cmds);

        [[nodiscard]] Rx::Optional<engine::renderer::TextureHandle> upload_texture_from_gltf(Uint32 texture_idx,
                                                                                            const tinygltf::Model& scene,
                                                                                            const ComPtr<ID3D12GraphicsCommandList4>& cmds);
    };
} // namespace sanity::editor::import
