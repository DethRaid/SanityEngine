#pragma once

#include "actor/actor.hpp"
#include "entt/entity/fwd.hpp"
#include "loading/asset_loader.hpp"
#include "renderer/handles.hpp"
#include "renderer/mesh_data_store.hpp"
#include "renderer/rhi/raytracing_structs.hpp"
#include "renderer/hlsl/standard_material.hpp"
#include "rx/core/map.h"
#include "rx/core/optional.h"
#include "rx/core/vector.h"
#include "tiny_gltf.h"

namespace sanity {
    namespace engine::renderer {
        class Renderer;
    } // namespace engine::renderer

    namespace editor {
        struct SceneImportSettings;

        namespace import {
            class SceneImporter {
            public:
                explicit SceneImporter(engine::renderer::Renderer& renderer_in);

                [[nodiscard]] Rx::Optional<entt::entity> import_gltf_scene(const std::filesystem::path& scene_path,
                                                                           const SceneImportSettings& import_settings,
                                                                           entt::registry& registry);

            private:
                struct GltfPrimitive {
                    engine::renderer::Mesh mesh{};

                    engine::renderer::RaytracingAsHandle ray_geo_handle{};

                    Int32 material_idx{-1};
                };

                struct GltfMesh {
                    Rx::Vector<GltfPrimitive> primitives;
                };

                tinygltf::TinyGLTF importer;

                engine::renderer::Renderer* renderer;

                // Textures, materials, and meshes that were loaded from the current GLTF file

                Rx::Map<Rx::String, engine::renderer::TextureHandle> loaded_textures;
                Rx::Vector<GltfMesh> meshes;
                Rx::Vector<engine::renderer::StandardMaterialHandle> materials;

                // Rx::Vector<engine::renderer::AnimationHandle> animations;

                [[nodiscard]] Rx::Vector<engine::renderer::StandardMaterialHandle> import_all_materials(const tinygltf::Model& scene,
                                                                                                        ID3D12GraphicsCommandList4* cmds);

                [[nodiscard]] Rx::Optional<engine::renderer::TextureHandle> import_texture(Int32 texture_idx,
                                                                                           const tinygltf::Model& scene,
                                                                                           ID3D12GraphicsCommandList4* cmds);

                [[nodiscard]] Rx::Vector<GltfMesh> import_all_meshes(const tinygltf::Model& scene, ID3D12GraphicsCommandList4* cmds) const;

                [[nodiscard]] static Rx::Optional<GltfPrimitive> get_data_from_primitive(const tinygltf::Primitive& primitive,
                                                                                         const tinygltf::Model& scene,
                                                                                         const engine::renderer::MeshUploader& uploader);

                [[nodiscard]] static Rx::Vector<Uint32> get_indices_from_primitive(const tinygltf::Primitive& primitive,
                                                                                   const tinygltf::Model& scene);

                [[nodiscard]] static Rx::Vector<StandardVertex> get_vertices_from_primitive(const tinygltf::Primitive& primitive,
                                                                                            const tinygltf::Model& scene);

                [[nodiscard]] entt::entity import_object_hierarchy(const tinygltf::Model& model,
                                                                   float import_scale,
                                                                   entt::registry& registry,
                                                                   ID3D12GraphicsCommandList4* cmds);
                void import_node_mesh(const tinygltf::Node& node,
                                      entt::registry& registry,
                                      ID3D12GraphicsCommandList4* cmds,
                                      entt::entity node_entity);

                void import_node_transform(const tinygltf::Node& node,
                                           const entt::entity& parent_entity,
                                           float import_scale,
                                           entt::registry& registry,
                                           engine::Actor& node_actor) const;

                void import_node_light(const tinygltf::Node& node,
                                       const tinygltf::Model& model,
                                       entt::registry& registry,
                                       entt::entity node_entity) const;

                entt::entity create_entity_for_node(const tinygltf::Node& node,
                                                    const entt::entity& parent_entity,
                                                    float import_scale,
                                                    const tinygltf::Model& model,
                                                    entt::registry& registry,
                                                    ID3D12GraphicsCommandList4* cmds);
            };
        } // namespace import
    }     // namespace editor
} // namespace sanity
