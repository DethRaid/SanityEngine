#pragma once

#include <loading/asset_loader.hpp>
#include <renderer/rhi/mesh_data_store.hpp>

#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include "renderer/handles.hpp"
#include "rx/core/map.h"
#include "rx/core/optional.h"
#include "rx/core/vector.h"
#include "tiny_gltf.h"

namespace sanity::editor {
    struct SceneImportSettings;
} // namespace sanity::editor

namespace sanity::engine::renderer {
    class Renderer;
} // namespace sanity::engine::renderer

namespace Rx {
    struct String;
}

namespace sanity::editor::import {
    class SceneImporter {
    public:
        explicit SceneImporter(engine::renderer::Renderer& renderer_in);

        [[nodiscard]] Rx::Optional<entt::entity> import_gltf_scene(const Rx::String& scene_path,
                                                                   const SceneImportSettings& import_settings,
                                                                   entt::registry& registry);

    private:
        struct GltfMesh {
            Rx::Vector<engine::renderer::Mesh> primitive_meshes;
        };

        tinygltf::TinyGLTF importer;

        engine::renderer::Renderer* renderer;

        Rx::Map<Rx::String, engine::renderer::TextureHandle> loaded_textures;

        [[nodiscard]] Rx::Vector<engine::renderer::StandardMaterialHandle> import_all_materials(const tinygltf::Model& scene,
                                                                                                ID3D12GraphicsCommandList4* cmds);

        [[nodiscard]] Rx::Optional<engine::renderer::TextureHandle> upload_texture_from_gltf(Uint32 texture_idx,
                                                                                             const tinygltf::Model& scene,
                                                                                             ID3D12GraphicsCommandList4* cmds);

        [[nodiscard]] Rx::Vector<GltfMesh> import_all_meshes(const tinygltf::Model& scene, ID3D12GraphicsCommandList4* cmds) const;

        [[nodiscard]] static Rx::Optional<engine::renderer::Mesh> get_data_from_primitive(const tinygltf::Primitive& primitive,
                                                                                          const tinygltf::Model& scene,
                                                                                          const engine::renderer::MeshUploader& uploader);

        [[nodiscard]] static Rx::Vector<Uint32> get_indices_from_primitive(const tinygltf::Primitive& primitive,
                                                                           const tinygltf::Model& scene);

        [[nodiscard]] static Rx::Vector<StandardVertex> get_vertices_from_primitive(const tinygltf::Primitive& primitive,
                                                                                    const tinygltf::Model& scene);
    };
} // namespace sanity::editor::import
