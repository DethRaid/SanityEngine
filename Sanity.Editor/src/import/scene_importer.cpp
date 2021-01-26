#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "scene_importer.hpp"

#include <ranges>

#include "Tracy.hpp"
#include "actor/actor.hpp"
#include "asset_registry/asset_registry.hpp"
#include "core/types.hpp"
#include "entity/entity_operations.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "renderer/renderer.hpp"
#include "renderer/standard_material.hpp"
#include "rx/core/log.h"
#include "rx/core/string.h"
#include "sanity_engine.hpp"

#define LOG_MISSING_ATTRIBUTE(attribute_name) logger->error("No attribute %s in primitive, aborting", (attribute_name))

namespace sanity::editor::import {
    RX_LOG("SceneImporter", logger);
    RX_LOG("\033[91mGltfImporter\033[0m", gltf_logger);

    constexpr const char* POSITION_ATTRIBUTE_NAME = "POSITION";
    constexpr const char* NORMAL_ATTRIBUTE_NAME = "NORMAL";
    constexpr const char* TEXCOORD_ATTRIBUTE_NAME = "TEXCOORD_0";

    constexpr const char* BASE_COLOR_TEXTURE_PARAM_NAME = "baseColorTexture";
    constexpr const char* BASE_COLOR_FACTOR_PARAM_NAME = "baseColorFactor";

    constexpr const char* EMISSIVE_TEXTURE_PARAM_NAME = "emissiveTexture";
    constexpr const char* EMISSIVE_FACTOR_PARAM_NAME = "emissiveFactor";

    constexpr const char* METALLIC_ROUGHNESS_TEXTURE_PARAM_NAME = "metallicRoughnessTexture";
    constexpr const char* METALLIC_FACTOR_PARAM_NAME = "metallicFactor";
    constexpr const char* ROUGHNESS_FACTOR_PARAM_NAME = "roughnessFactor";

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
            return reinterpret_cast<const DataType*>(buffer.data.data() + buffer_view.byteOffset + accessor.byteOffset);
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

        Rx::Vector<Uint32> flip_triangle_winding_order(const Rx::Vector<Uint32>& indices) {
            if(indices.size() % 3 != 0) {
                logger->error("Cannot flip winding order: triangle index buffer must be a multiple of three");
                return {};
            }

            Rx::Vector<Uint32> flipped_indices;
            flipped_indices.reserve(indices.size());
            for(Size i = 0; i < indices.size(); i += 3) {
                flipped_indices.push_back(indices[i + 2]);
                flipped_indices.push_back(indices[i + 1]);
                flipped_indices.push_back(indices[i]);
            }

            return flipped_indices;
        }
    } // namespace detail

    SceneImporter::SceneImporter(engine::renderer::Renderer& renderer_in) : renderer{&renderer_in} {}

    Rx::Optional<entt::entity> SceneImporter::import_gltf_scene(const std::filesystem::path& scene_path,
                                                                const SceneImportSettings& import_settings,
                                                                entt::registry& registry) {
        ZoneScoped;

        tinygltf::Model scene;
        std::string err;
        std::string warn;
        bool success;
        if(scene_path.extension() == ".glb") {
            ZoneScopedN("Load .glb");
            success = importer.LoadBinaryFromFile(&scene, &err, &warn, scene_path.string());

        } else if(scene_path.extension() == ".gltf") {
            ZoneScopedN("Load .gltf");
            success = importer.LoadASCIIFromFile(&scene, &err, &warn, scene_path.string());

        } else {
            gltf_logger->error("Invalid scene file %s", scene_path);
            return Rx::nullopt;
        }

        if(!warn.empty()) {
            gltf_logger->warning("Warnings when reading %s: %s", scene_path, warn);
        }
        if(!err.empty()) {
            gltf_logger->error("Errors when reading %s: %s", scene_path, err);
        }
        if(!success) {
            gltf_logger->error("Could not read scene %s", scene_path);
            return Rx::nullopt;
        }

        gltf_logger->info("Loaded scene %s", scene_path);

        // How to import a scene?
        // First, load all the meshes, textures, materials, and other primitives;

        auto& backend = renderer->get_render_backend();
        // backend.begin_frame_capture();
        auto cmds = backend.create_command_list();

        if(import_settings.import_meshes) {
            meshes = import_all_meshes(scene, cmds.Get());
        }

        if(import_settings.import_materials) {
            materials = import_all_materials(scene, cmds.Get());
        }

        // Then, walk the node hierarchy, creating an hierarchy of entt::entities
        Rx::Optional<entt::entity> scene_entity{Rx::nullopt};
        if(import_settings.import_object_hierarchy) {
            scene_entity = import_object_hierarchy(scene, import_settings.scaling_factor, registry, cmds.Get());
        }

        backend.submit_command_list(Rx::Utility::move(cmds));

        // Finally, return the root entity
        return scene_entity;
    }

    Rx::Vector<engine::renderer::StandardMaterialHandle> SceneImporter::import_all_materials(const tinygltf::Model& scene,
                                                                                             ID3D12GraphicsCommandList4* cmds) {
        ZoneScoped;

        Rx::Vector<engine::renderer::StandardMaterialHandle> imported_materials;

        for(const auto& material : scene.materials) {
            ZoneScopedN(material.name.c_str());

            gltf_logger->info("Importing material %s", material.name.c_str());

            engine::renderer::StandardMaterial sanity_material{};

            auto has_base_color_value{false};
            auto has_normal_value{false};
            auto has_metallic_value{false};
            auto has_roughness_value{false};
            auto has_emission_value{false};

            // Extract the texture indices

            int base_color_texture_idx{-1};
            int metalness_roughness_texture_idx{-1};
            int normal_texture_idx{-1};
            int emission_texture_idx{-1};

            for(const auto& [param_name, param] : material.values) {
                if(param_name == BASE_COLOR_TEXTURE_PARAM_NAME) {
                    base_color_texture_idx = param.TextureIndex();

                } else if(param_name == METALLIC_ROUGHNESS_TEXTURE_PARAM_NAME) {
                    metalness_roughness_texture_idx = param.TextureIndex();

                } else if(param_name == BASE_COLOR_FACTOR_PARAM_NAME) {
                    const auto color = param.ColorFactor();
                    sanity_material.base_color_value = glm::make_vec4(color.data());
                    has_base_color_value = true;

                } else if(param_name == METALLIC_FACTOR_PARAM_NAME) {
                    sanity_material.metallic_roughness_value.g = static_cast<float>(param.Factor());
                    has_metallic_value = true;

                } else if(param_name == ROUGHNESS_FACTOR_PARAM_NAME) {
                    sanity_material.metallic_roughness_value.b = static_cast<float>(param.Factor());
                    has_roughness_value = true;
                }
            }

            for(const auto& [param_name, param] : material.additionalValues) {
                if(param_name == NORMAL_TEXTURE_PARAM_NAME) {
                    normal_texture_idx = param.TextureIndex();

                } else if(param_name == EMISSIVE_TEXTURE_PARAM_NAME) {
                    emission_texture_idx = param.TextureIndex();

                } else if(param_name == EMISSIVE_FACTOR_PARAM_NAME) {
                    const auto color = param.ColorFactor();
                    sanity_material.emission_value = {glm::make_vec3(color.data()), sanity_material.emission_value.w};
                }
            }

            if(base_color_texture_idx == -1) {
                if(has_base_color_value) {
                    logger->verbose("No base color texture. Falling back to base color value");
                } else {
                    logger->warning("No base color property on material %s", material.name);
                }

            } else {
                if(const auto handle = get_sanity_handle_to_texture(base_color_texture_idx, scene, cmds); handle.has_value()) {
                    sanity_material.base_color_texture = *handle;

                } else {
                    gltf_logger->error("Could not import texture %d (from material parameter %s) into SanityEngine",
                                       base_color_texture_idx,
                                       BASE_COLOR_TEXTURE_PARAM_NAME);
                    sanity_material.base_color_texture = renderer->get_pink_texture();
                }
            }

            if(metalness_roughness_texture_idx == -1) {
                if(has_metallic_value && has_roughness_value) {
                    logger->verbose("No metalness/roughness texture. Falling back to metalness/roughness value");
                } else {
                    logger->warning("No metalness/roughness property on material %s", material.name);
                }

            } else {
                if(const auto handle = get_sanity_handle_to_texture(metalness_roughness_texture_idx, scene, cmds); handle.has_value()) {
                    sanity_material.metallic_roughness_texture = *handle;

                } else {
                    gltf_logger->error("Could not import texture %d (from material parameter %s) into SanityEngine",
                                       metalness_roughness_texture_idx,
                                       METALLIC_ROUGHNESS_TEXTURE_PARAM_NAME);
                    sanity_material.metallic_roughness_texture = renderer->get_pink_texture();
                }
            }

            if(normal_texture_idx == -1) {
                logger->verbose("No normal texture. Falling back to mesh normals");

            } else {
                if(const auto handle = get_sanity_handle_to_texture(normal_texture_idx, scene, cmds); handle.has_value()) {
                    sanity_material.normal_texture = *handle;

                } else {
                    gltf_logger->error("Could not import texture %d (from material parameter %s) into SanityEngine",
                                       normal_texture_idx,
                                       NORMAL_TEXTURE_PARAM_NAME);
                    sanity_material.normal_texture = renderer->get_pink_texture();
                }
            }

            if(emission_texture_idx == -1) {
                if(has_emission_value) {
                    logger->verbose("No emission texture. Falling back to emission value");
                } else {
                    logger->info("No emission property on material %s", material.name);
                }

            } else {
                if(const auto handle = get_sanity_handle_to_texture(emission_texture_idx, scene, cmds); handle.has_value()) {
                    sanity_material.emission_texture = *handle;

                } else {
                    gltf_logger->error("Could not import texture %d (from material parameter %s) into SanityEngine",
                                       emission_texture_idx,
                                       EMISSIVE_TEXTURE_PARAM_NAME);
                    sanity_material.normal_texture = renderer->get_pink_texture();
                }
            }

            // Allocate material on GPU

            const auto handle = renderer->allocate_standard_material(sanity_material);
            imported_materials.push_back(handle);
        }

        return imported_materials;
    }

    Rx::Optional<engine::renderer::TextureHandle> SceneImporter::get_sanity_handle_to_texture(const Int32 texture_idx,
                                                                                              const tinygltf::Model& scene,
                                                                                              ID3D12GraphicsCommandList4* cmds) {
        static Byte* padding_buffer{nullptr};

        ZoneScoped;

        if(texture_idx < 0) {
            return Rx::nullopt;
        }

        const auto& texture = scene.textures[texture_idx];
        const auto& texture_name = Rx::String{texture.name.c_str()};

        if(!texture_name.is_empty()) {
            if(const auto* texture_ptr = loaded_textures.find(texture_name); texture_ptr != nullptr) {
                return *texture_ptr;
            }
        }

        if(texture.source < 0) {
            gltf_logger->error("Texture %s has an invalid source", texture.name.c_str());
            return Rx::nullopt;
        }

        // We only support three- or four-channel textures, with eight bits per chanel
        const auto& source_image = scene.images[texture.source];
        if(source_image.component != 3 && source_image.component != 4) {
            gltf_logger->error("Source image %s does not have either three or four components", source_image.name.c_str());
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

            image_data = padding_buffer;
        }

        const auto image_name = texture_name.is_empty() ? "Imported GLTF texture" : texture_name;

        const auto create_info = engine::renderer::ImageCreateInfo{.name = image_name,
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
        ZoneScoped;

        auto& mesh_store = renderer->get_static_mesh_store();
        const auto uploader = mesh_store.begin_adding_meshes(cmds);

        Rx::Vector<GltfMesh> imported_meshes;
        imported_meshes.reserve(scene.meshes.size());

        for(const auto& mesh : scene.meshes) {
            ZoneScopedN(mesh.name.c_str());

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
        ZoneScoped;

        const auto indices = get_indices_from_primitive(primitive, scene);
        const auto vertices = get_vertices_from_primitive(primitive, scene);

        if(indices.is_empty() || vertices.is_empty()) {
            gltf_logger->error("Could not read primitive data");
            return Rx::nullopt;
        }

        const auto fixed_indices = detail::flip_triangle_winding_order(indices);

        const auto mesh = uploader.add_mesh(vertices, fixed_indices);
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
        Rx::String all_attributes;
        for(const auto& attribute_name : primitive.attributes | std::views::keys) {
            all_attributes = Rx::String::format("%s, %s", all_attributes, attribute_name.c_str());
        }
        gltf_logger->verbose("Primitive has attributes %s", all_attributes);

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

        const auto& positions_accessor = scene.accessors[positions_itr->second];

        Rx::Vector<StandardVertex> vertices;
        vertices.reserve(positions_accessor.count);

        for(Uint32 i = 0; i < positions_accessor.count; i++) {
            // Hope that all the buffers have the same size... They should...

            const auto location = *position_read_ptr;
            const auto normal = *normal_read_ptr;
            Vec2f texcoord;
            if(texcoord_read_ptr != nullptr) {
                texcoord = *texcoord_read_ptr;
                texcoord.y = 1.0f - texcoord.y; // Convert OpenGL-style texcoords to DirectX-style
            }

            vertices.push_back(
                StandardVertex{.location = location, .normal = {normal.x, normal.y, -normal.z}, .texcoord = {texcoord.x, -texcoord.y}});

            position_read_ptr++;
            normal_read_ptr++;
            texcoord_read_ptr++;
        }

        return vertices;
    }

    entt::entity SceneImporter::import_object_hierarchy(const tinygltf::Model& model,
                                                        const float import_scale,
                                                        entt::registry& registry,
                                                        ID3D12GraphicsCommandList4* cmds) {
        ZoneScoped;

        // Assume that the files we'll be importing have a single scene
        const auto& default_scene = model.scenes[model.defaultScene];

        // Create an entity for the scene and reference one of its components
        const auto& scene_entity = engine::create_actor(registry, "Imported scene");

        // Add entities for all the nodes in the scene, and all their children
        for(const auto node_idx : default_scene.nodes) {
            const auto& node = model.nodes[node_idx];
            create_entity_for_node(node, scene_entity.entity, import_scale, model, registry, cmds);
        }

        return scene_entity.entity;
    }

    entt::entity SceneImporter::create_entity_for_node(const tinygltf::Node& node,
                                                       const entt::entity& parent_entity,
                                                       const float import_scale,
                                                       const tinygltf::Model& model,
                                                       entt::registry& registry,
                                                       ID3D12GraphicsCommandList4* cmds) {
        ZoneScoped;

        auto& node_actor = engine::create_actor(registry, node.name.empty() ? "New Node" : node.name.c_str());
        auto node_entity = node_actor.entity;

        auto& node_transform_component = node_actor.get_component<engine::TransformComponent>();
        auto& node_transform = node_transform_component.transform;

        if(!node.matrix.empty()) {
            const auto transform_matrix = glm::make_mat4(node.matrix.data());

            node_transform.location = transform_matrix[3];

            auto i = glm::mat3{transform_matrix};

            node_transform.scale.x = static_cast<float>(length(i[0]));
            node_transform.scale.y = static_cast<float>(length(i[1]));
            node_transform.scale.z = static_cast<float>(glm::sign(determinant(i)) * length(i[2]));

            i[0] /= node_transform.scale.x;
            i[1] /= node_transform.scale.y;
            i[2] /= node_transform.scale.z;

            node_transform.rotation = toQuat(i);

        } else {
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

            node_transform.scale = glm::vec3{import_scale};

            if(node.scale.size() == 3) {
                node_transform.scale.x *= static_cast<Float32>(node.scale[0]);
                node_transform.scale.y *= static_cast<Float32>(node.scale[1]);
                node_transform.scale.z *= static_cast<Float32>(node.scale[2]);
            }
        }

        logger->verbose("Created node %s with transform translation=%s rotation=%s scale=%s",
                        node.name.empty() ? "New Entity" : node.name,
                        node_transform.location,
                        node_transform.rotation,
                        node_transform.scale);

        if(registry.valid(parent_entity)) {
            node_transform_component.parent = parent_entity;

            auto& parent_transform = registry.get<engine::TransformComponent>(parent_entity);
            parent_transform.children.push_back(node_actor.entity);
        }

        if(node.mesh > -1 && static_cast<Size>(node.mesh) < meshes.size()) {
            const auto& mesh = meshes[node.mesh];
            Uint32 i{0};

            auto mesh_adder = renderer->get_static_mesh_store().begin_adding_meshes(cmds);
            mesh_adder.prepare_for_raytracing_geometry_build();

            const auto vertex_buffer = renderer->get_static_mesh_store().get_vertex_buffer();
            const auto index_buffer = renderer->get_static_mesh_store().get_index_buffer();

            Rx::Vector<engine::renderer::RaytracingObject> raytracing_objects;

            mesh.primitives.each_fwd([&](const GltfPrimitive& primitive) {
                // Create entity and components
                const auto primitive_node_name = Rx::String::format("%s primitive %d", node.name, i);
                auto& primitive_actor = engine::create_actor(registry, primitive_node_name);

                auto& primitive_transform_component = primitive_actor.get_component<engine::TransformComponent>();
                primitive_transform_component.parent = node_entity;

                auto& parent_transform_component = registry.get<engine::TransformComponent>(node_entity);
                parent_transform_component.children.push_back(primitive_actor.entity);

                auto& renderable = primitive_actor.add_component<engine::renderer::StandardRenderableComponent>();
                renderable.mesh = primitive.mesh;
                renderable.material = materials[primitive.material_idx];

                // Build raytracing acceleration structure. We make a separate BLAS for each primitive because they
                // might have separate materials. However, this will create too many BLASs if primitives share
                // materials. This will be addressed in a future revision
                Rx::Vector<engine::renderer::PlacedMesh> meshes_for_raytracing;

                meshes_for_raytracing.emplace_back(primitive.mesh, parent_transform_component.transform);

                const auto as_handle = renderer->create_raytracing_geometry(vertex_buffer, index_buffer, meshes_for_raytracing, cmds);

                auto& raytracing_object_component = primitive_actor.add_component<engine::renderer::RaytracingObjectComponent>();
                raytracing_object_component.as_handle = as_handle;

                const auto ray_material = engine::renderer::RaytracingMaterial{.handle = renderable.material.index};

                const auto ray_object = engine::renderer::RaytracingObject{.as_handle = as_handle,
                                                                           .material = ray_material,
                                                                           .transform = primitive_transform_component.get_world_matrix(
                                                                               registry)};
                //,
                //.transform = cached_transform.to_matrix()};
                raytracing_objects.push_back(ray_object);

                i++;
            });

            renderer->add_raytracing_objects_to_scene(raytracing_objects);

        } else {
            logger->error("Node %s references invalid mesh %d", node.name.c_str(), node.mesh);
        }

        // TODO: Lights

        // Child nodes
        for(const auto child_node_idx : node.children) {
            if(child_node_idx < 0 || static_cast<Size>(child_node_idx) > model.nodes.size()) {
                // Invalid node index
                continue;
            }

            const auto& child_node = model.nodes.at(child_node_idx);
            create_entity_for_node(child_node, node_entity, import_scale, model, registry, cmds);
        }

        return node_entity;
    }
} // namespace sanity::editor::import
