#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "scene_importer.hpp"

#include "Tracy.hpp"
#include "asset_registry/asset_registry.hpp"
#include "core/types.hpp"
#include "entity/entity_operations.hpp"
#include "renderer/renderer.hpp"
#include "renderer/standard_material.hpp"
#include "rx/core/log.h"
#include "rx/core/string.h"
#include "sanity_engine.hpp"

#define LOG_MISSING_ATTRIBUTE(attribute_name) logger->error("No attribute %s in primitive, aborting", (attribute_name))

namespace sanity::editor::import {
    RX_LOG("SceneImporter", logger);
    RX_LOG("GltfImporter", gltf_logger);

    constexpr const char* POSITION_ATTRIBUTE_NAME = "POSITION";
    constexpr const char* NORMAL_ATTRIBUTE_NAME = "NORMAL";
    constexpr const char* TEXCOORD_ATTRIBUTE_NAME = "TEXCOORD_0";

    constexpr const char* BASE_COLOR_TEXTURE_PARAM_NAME = "baseColorTexture";
    constexpr const char* METALLIC_ROUGHNESS_TEXTURE_PARAM_NAME = "metallicRoughnessTexture";
    constexpr const char* NORMAL_TEXTURE_PARAM_NAME = "normalTexture";

    namespace detail {
        template <typename DataType>
        [[nodiscard]] const DataType* get_pointer_to_accessor_data(const int accessor_idx, const tinygltf::Model& scene) {
            if(accessor_idx < 0) {
                gltf_logger->error("Accessor index %d is less than 0, aborting", accessor_idx);
                return nullptr;

            } else if(accessor_idx >= scene.accessors.size()) {
                gltf_logger->error("Accessor index %d is greater than number of accessors %d, aborting",
                                   accessor_idx,
                                   scene.accessors.size());
                return nullptr;
            }

            const auto& accessor = scene.accessors[accessor_idx];
            if(accessor.bufferView < 0) {
                gltf_logger->error("Buffer view index %d is less than zero, aborting", accessor.bufferView);
                return nullptr;

            } else if(accessor.bufferView >= scene.bufferViews.size()) {
                gltf_logger->error("Buffer view index %d is greater than number of buffer views %d, aborting",
                                   accessor.bufferView,
                                   scene.bufferViews.size());
                return nullptr;
            }

            const auto& buffer_view = scene.bufferViews[accessor.bufferView];
            if(buffer_view.buffer < 0) {
                gltf_logger->error("Buffer index is less than zero, aborting");
                return nullptr;

            } else if(buffer_view.buffer > scene.buffers.size()) {
                gltf_logger->error("Buffer index %d is greater than number of buffers %d, aborting",
                                   buffer_view.buffer,
                                   scene.buffers.size());
                return nullptr;
            }

            const auto& buffer = scene.buffers[buffer_view.buffer];
            return reinterpret_cast<const DataType*>(buffer.data.data() + buffer_view.byteLength);
        }

        template <typename IndexType>
        Rx::Vector<Uint32> get_indices_from_primitive_impl(const Int32 indices_accessor_idx, const tinygltf::Model& scene) {
            const auto* index_read_ptr = get_pointer_to_accessor_data<Byte>(indices_accessor_idx, scene);

            Rx::Vector<Uint32> indices;

            const auto& index_accessor = scene.accessors[indices_accessor_idx];
            indices.reserve(index_accessor.count);
            for(Uint32 i = 0; i < index_accessor.count; i++) {
                const auto* index_read_ptr_typed = reinterpret_cast<const IndexType*>(index_read_ptr);
                const auto index = *index_read_ptr_typed;
                indices.push_back(static_cast<Uint32>(index));
                index_read_ptr += sizeof(IndexType);
            }

            return indices;
        }
    } // namespace detail

    SceneImporter::SceneImporter(engine::renderer::Renderer& renderer_in) : renderer{&renderer_in} {}

    Rx::Optional<entt::entity> SceneImporter::import_gltf_scene(const Rx::String& scene_path,
                                                                const SceneImportSettings& import_settings,
                                                                entt::registry& registry) {

        tinygltf::Model scene;
        std::string err;
        std::string warn;
        bool success;
        if(scene_path.ends_with(".glb")) {
            success = importer.LoadBinaryFromFile(&scene, &err, &warn, scene_path.data());

        } else if(scene_path.ends_with(".gltf")) {
            success = importer.LoadASCIIFromFile(&scene, &err, &warn, scene_path.data());

        } else {
            gltf_logger->error("Invalid scene file %s", scene_path);
            return Rx::nullopt;
        }

        if(!warn.empty()) {
            gltf_logger->warning("Warnings when reading %s: %s", scene_path, warn.c_str());
        }
        if(!err.empty()) {
            gltf_logger->error("Errors when reading %s: %s", scene_path, err.c_str());
        }
        if(!success) {
            gltf_logger->error("Could not read scene %s", scene_path);
            return Rx::nullopt;
        }

        // How to import a scene?
        // First, load all the meshes, textures, materials, and other primitives;

        auto& backend = renderer->get_render_backend();
        auto cmds = backend.create_command_list();

        if(import_settings.import_meshes) {
            meshes = import_all_meshes(scene, cmds.Get());
        }

        if(import_settings.import_materials) {
            materials = import_all_materials(scene, cmds.Get());
        }

        backend.submit_command_list(Rx::Utility::move(cmds));

        // Then, walk the node hierarchy, creating an hierarchy of entt::entities
        if(import_settings.import_object_hierarchy) {
            const auto scene_entity = import_object_hierarchy(scene, registry);
        }

        // Finally, return the root entity

        return {};
    }

    Rx::Vector<engine::renderer::StandardMaterialHandle> SceneImporter::import_all_materials(const tinygltf::Model& scene,
                                                                                             ID3D12GraphicsCommandList4* cmds) {
        ZoneScoped;

        Rx::Vector<engine::renderer::StandardMaterialHandle> materials;

        for(const auto& material : scene.materials) {
            gltf_logger->info("Importing material %s", material.name.c_str());

            engine::renderer::StandardMaterial sanity_material{};

            // Extract the texture indices

            int base_color_texture_idx{-1};
            int metallic_roughness_texture_idx{-1};
            int normal_texture_idx{-1};

            for(const auto& [param_name, param] : material.values) {
                gltf_logger->verbose("Value %s has texture index %d", param_name.c_str(), param.TextureIndex());
                if(param_name == BASE_COLOR_TEXTURE_PARAM_NAME) {
                    base_color_texture_idx = param.TextureIndex();

                } else if(param_name == METALLIC_ROUGHNESS_TEXTURE_PARAM_NAME) {
                    metallic_roughness_texture_idx = param.TextureIndex();
                }
            }

            for(const auto& [param_name, param] : material.additionalValues) {
                gltf_logger->verbose("Additional value %s has texture index %d", param_name.c_str(), param.TextureIndex());
                if(param_name == NORMAL_TEXTURE_PARAM_NAME) {
                    normal_texture_idx = param.TextureIndex();
                }
            }

            // Validate

            if(base_color_texture_idx == -1) {
                gltf_logger->error("Parameter %s on material %s isn't a valid texture",
                                   BASE_COLOR_TEXTURE_PARAM_NAME,
                                   material.name.c_str());
                continue;
            }
            if(metallic_roughness_texture_idx == -1) {
                gltf_logger->error("Parameter %s on material %s is not a valid texture",
                                   METALLIC_ROUGHNESS_TEXTURE_PARAM_NAME,
                                   material.name.c_str());
                continue;
            }
            if(normal_texture_idx == -1) {
                gltf_logger->error("Parameter %s on material %s is not a valid texture", NORMAL_TEXTURE_PARAM_NAME, material.name.c_str());
                continue;
            }

            // Load the textures

            if(const auto handle = get_sanity_handle_to_texture_in_gltf(static_cast<Uint32>(base_color_texture_idx), scene, cmds);
               handle.has_value()) {
                sanity_material.base_color = *handle;
            }

            if(const auto handle = get_sanity_handle_to_texture_in_gltf(static_cast<Uint32>(metallic_roughness_texture_idx), scene, cmds);
               handle.has_value()) {
                sanity_material.metallic_roughness = *handle;
            }
            if(const auto handle = get_sanity_handle_to_texture_in_gltf(static_cast<Uint32>(normal_texture_idx), scene, cmds);
               handle.has_value()) {
                sanity_material.normal = *handle;
            }

            // Allocate material on GPU

            const auto handle = renderer->allocate_standard_material(sanity_material);
            materials.push_back(handle);
        }

        return materials;
    }

    Rx::Optional<engine::renderer::TextureHandle> SceneImporter::get_sanity_handle_to_texture_in_gltf(const Uint32 texture_idx,
                                                                                                      const tinygltf::Model& scene,
                                                                                                      ID3D12GraphicsCommandList4* cmds) {
        static Byte* padding_buffer{nullptr};

        ZoneScoped;

        const auto& texture = scene.textures[texture_idx];
        const auto& texture_name = Rx::String{texture.name.c_str()};

        if(const auto* texture_ptr = loaded_textures.find(texture_name); texture_ptr != nullptr) {
            return *texture_ptr;
        }

        if(texture.source < 0) {
            gltf_logger->error("Texture %s has an invalid source", texture.name.c_str());
            return Rx::nullopt;
        }

        // We only support three- or four-channel textures, with eight bits per chanel
        const auto& source_image = scene.images[texture.source];
        if(source_image.component != 3 && source_image.component != 4) {
            gltf_logger->error("Source image %s does not have either 3 or four components", source_image.name.c_str());
            return Rx::nullopt;
        }
        if(source_image.bits != 8) {
            gltf_logger->error("Source image does not have eight bits per component. Unable to load");
            return Rx::nullopt;
        }

        const void* image_data = &source_image.image.at(0);
        if(source_image.component == 3) {
            // We have to pad out the data because GPUs don't like multiples of 3
            padding_buffer = RX_SYSTEM_ALLOCATOR.reallocate(padding_buffer,
                                                            static_cast<std::size_t>(source_image.width) * source_image.height * 4);

            std::size_t read_idx{0};
            Size write_idx{0};

            for(Uint32 pixel_idx = 0; pixel_idx < static_cast<Uint32>(source_image.width * source_image.height); pixel_idx++) {
                padding_buffer[write_idx] = source_image.image[read_idx];
                padding_buffer[write_idx + 1] = source_image.image[read_idx + 3];
                padding_buffer[write_idx + 2] = source_image.image[read_idx + 2];
                padding_buffer[write_idx + 3] = 0xFF;

                read_idx++;
                write_idx++;
            }
        }

        const auto create_info = engine::renderer::ImageCreateInfo{.name = source_image.name.c_str(),
                                                                   .usage = engine::renderer::ImageUsage::SampledImage,
                                                                   .format = engine::renderer::ImageFormat::Rgba8,
                                                                   .width = static_cast<Uint32>(source_image.width),
                                                                   .height = static_cast<Uint32>(source_image.height)};

        const auto handle = renderer->create_image(create_info, image_data, cmds);
        loaded_textures.insert(texture_name, handle);

        return handle;
    }

    Rx::Vector<SceneImporter::GltfMesh> SceneImporter::import_all_meshes(const tinygltf::Model& scene,
                                                                         ID3D12GraphicsCommandList4* cmds) const {
        auto& mesh_store = renderer->get_static_mesh_store();
        const auto uploader = mesh_store.begin_adding_meshes(cmds);

        Rx::Vector<GltfMesh> imported_meshes;
        imported_meshes.reserve(scene.meshes.size());

        for(const auto& mesh : scene.meshes) {
            gltf_logger->info("Importing mesh %s", mesh.name.c_str());

            GltfMesh imported_mesh{};
            imported_mesh.primitives.reserve(mesh.primitives.size());

            for(Uint32 primitive_idx = 0; primitive_idx < mesh.primitives.size(); primitive_idx++) {
                gltf_logger->verbose("Importing primitive %d", primitive_idx);

                const auto& primitive = mesh.primitives[primitive_idx];

                const auto& primitive_mesh = get_data_from_primitive(primitive, scene, uploader);
                if(!primitive_mesh) {
                    gltf_logger->error("Could not read data for primitive %d in mesh %s", primitive_idx, mesh.name.c_str());
                    continue;
                }

                imported_mesh.primitives.push_back(*primitive_mesh);
            }

            imported_meshes.push_back(imported_mesh);
        }

        return imported_meshes;
    }

    Rx::Optional<SceneImporter::GltfPrimitive> SceneImporter::get_data_from_primitive(const tinygltf::Primitive& primitive,
                                                                                      const tinygltf::Model& scene,
                                                                                      const engine::renderer::MeshUploader& uploader) {
        const auto indices = get_indices_from_primitive(primitive, scene);
        const auto vertices = get_vertices_from_primitive(primitive, scene);

        if(indices.is_empty() || vertices.is_empty()) {
            gltf_logger->error("Could not read primitive data");
            return Rx::nullopt;
        }

        const auto mesh = uploader.add_mesh(vertices, indices);
        return GltfPrimitive{.mesh = mesh, .material_idx = primitive.material};
    }

    Rx::Vector<Uint32> SceneImporter::get_indices_from_primitive(const tinygltf::Primitive& primitive, const tinygltf::Model& scene) {
        const auto& index_accessor = scene.accessors[primitive.indices];

        if(index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_BYTE) {
            return detail::get_indices_from_primitive_impl<Int8>(primitive.indices, scene);

        } else if(index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            return detail::get_indices_from_primitive_impl<Uint8>(primitive.indices, scene);

        } else if(index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_SHORT) {
            return detail::get_indices_from_primitive_impl<Int16>(primitive.indices, scene);

        } else if(index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            return detail::get_indices_from_primitive_impl<Uint16>(primitive.indices, scene);

        } else if(index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_INT) {
            return detail::get_indices_from_primitive_impl<Int32>(primitive.indices, scene);

        } else if(index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            return detail::get_indices_from_primitive_impl<Uint32>(primitive.indices, scene);

        } else if(index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
            return detail::get_indices_from_primitive_impl<Float32>(primitive.indices, scene);

        } else if(index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_DOUBLE) {
            return detail::get_indices_from_primitive_impl<Float64>(primitive.indices, scene);
        }

        gltf_logger->error("Unrecognized component type %d in index accessor", index_accessor.componentType);

        return {};
    }

    Rx::Vector<StandardVertex> SceneImporter::get_vertices_from_primitive(const tinygltf::Primitive& primitive,
                                                                          const tinygltf::Model& scene) {
        const auto& positions_itr = primitive.attributes.find(POSITION_ATTRIBUTE_NAME);
        if(positions_itr == primitive.attributes.end()) {
            LOG_MISSING_ATTRIBUTE(POSITION_ATTRIBUTE_NAME);

            return {};
        }

        const auto* position_read_ptr = detail::get_pointer_to_accessor_data<Vec3f>(positions_itr->second, scene);
        if(position_read_ptr == nullptr) {
            logger->error("Could not get a pointer to the vertex positions");
            return {};
        }

        const auto* normal_read_ptr = [&]() -> const Vec3f* {
            const auto& normals_itr = primitive.attributes.find(NORMAL_ATTRIBUTE_NAME);
            if(normals_itr == primitive.attributes.end()) {
                LOG_MISSING_ATTRIBUTE(NORMAL_ATTRIBUTE_NAME);

                return nullptr;
            }

            return detail::get_pointer_to_accessor_data<Vec3f>(normals_itr->second, scene);
        }();
        if(normal_read_ptr == nullptr) {
            logger->error("Could not get a pointer to the vertex normals");
            return {};
        }

        const auto* texcoord_read_ptr = [&]() -> const Vec2f* {
            const auto& texcoord_itr = primitive.attributes.find(TEXCOORD_ATTRIBUTE_NAME);
            if(texcoord_itr == primitive.attributes.end()) {
                LOG_MISSING_ATTRIBUTE(TEXCOORD_ATTRIBUTE_NAME);

                return nullptr;
            }

            return detail::get_pointer_to_accessor_data<Vec2f>(texcoord_itr->second, scene);
        }();
        if(texcoord_read_ptr == nullptr) {
            logger->error("Could not get a pointer to the vertex texcoords");
            return {};
        }

        const auto& positions_accessor = scene.accessors[positions_itr->second];

        Rx::Vector<StandardVertex> vertices;
        vertices.reserve(positions_accessor.count);

        for(Uint32 i = 0; i < positions_accessor.count; i++) {
            // Hope that all the buffers have the same size... They should...

            vertices.push_back(StandardVertex{.position = *position_read_ptr, .normal = *normal_read_ptr, .texcoord = *texcoord_read_ptr});

            position_read_ptr++;
            normal_read_ptr++;
            texcoord_read_ptr++;
        }

        return vertices;
    }

    entt::entity SceneImporter::import_object_hierarchy(const tinygltf::Model& scene, entt::registry& registry) {
        // Assume that the files we'll be importing have a single scene
        const auto& default_scene = scene.scenes[scene.defaultScene];

        // Create an entity for the scene and reference one of its components
        const auto& scene_entity = entity::create_base_editor_entity("Imported scene", registry);
        entity::add_component<engine::TransformComponent>(scene_entity, registry);

        entity::add_component<engine::HierarchyComponent>(scene_entity, registry);

        // Add entities for all the nodes in the scene, and all their children
        for(const auto node_idx : default_scene.nodes) {
            const auto& node = scene.nodes[node_idx];

            const auto node_entity = create_entity_for_node(node, scene_entity, registry);

            // Get the component every loop so that we don't hold on to a bad reference
            auto& scene_hierarchy = registry.get<engine::HierarchyComponent>(scene_entity);
            scene_hierarchy.children.push_back(node_entity);
        }

        return scene_entity;
    }

    entt::entity SceneImporter::create_entity_for_node(const tinygltf::Node& node,
                                                       const entt::entity& parent_entity,
                                                       entt::registry& registry) {

        const auto node_entity = entity::create_base_editor_entity(node.name.c_str(), registry);

        auto& node_hierarchy = entity::add_component<engine::HierarchyComponent>(node_entity, registry);
        node_hierarchy.parent = parent_entity;

        auto& node_transform = entity::add_component<engine::TransformComponent>(node_entity, registry);
        if(node.translation.size() == 3) {
            node_transform.location.x = static_cast<Float32>(node.translation[0]);
            node_transform.location.y = static_cast<Float32>(node.translation[1]);
            node_transform.location.z = static_cast<Float32>(node.translation[2]);
        }

        if(node.rotation.size() == 4) {
            node_transform.rotation.x = static_cast<Float32>(node.rotation[0]);
            node_transform.rotation.y = static_cast<Float32>(node.rotation[1]);
            node_transform.rotation.z = static_cast<Float32>(node.rotation[2]);
            node_transform.rotation.w = static_cast<Float32>(node.rotation[3]);
        }

        if(node.scale.size() == 3) {
            node_transform.scale.x = static_cast<Float32>(node.scale[0]);
            node_transform.scale.y = static_cast<Float32>(node.scale[1]);
            node_transform.scale.z = static_cast<Float32>(node.scale[2]);
        }

        if(node.mesh > -1 && node.mesh < meshes.size()) {
            const auto& mesh = meshes[node.mesh];
            Uint32 i{0};

            mesh.primitives.each_fwd([&](const GltfPrimitive& primitive) {
                const auto primitive_node_name = Rx::String::format("%s primitive %d", node.name.c_str(), i);
                const auto primitive_entity = entity::create_base_editor_entity(primitive_node_name, registry);

                entity::add_component<engine::TransformComponent>(primitive_entity, registry);

                auto& primitive_hierarchy_component = entity::add_component<engine::HierarchyComponent>(primitive_entity, registry);
                primitive_hierarchy_component.parent = node_entity;

                auto& parent_hierarchy_component = registry.get<engine::HierarchyComponent>(node_entity);
                parent_hierarchy_component.children.push_back(primitive_entity);

                auto& renderable = entity::add_component<engine::renderer::StandardRenderableComponent>(primitive_entity, registry);
                renderable.mesh = primitive.mesh;
                renderable.material = materials[primitive.material_idx];

                i++;
            });

        } else {
            logger->error("Node %s references invalid mesh %d", node.name.c_str(), node.mesh);
        }

        // TODO: Lights

        return node_entity;
    }
} // namespace sanity::editor::import
