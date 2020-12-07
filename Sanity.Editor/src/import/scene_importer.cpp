#include "scene_importer.hpp"

#define TINYGLTF_IMPLEMENTATION

#include <sanity_engine.hpp>

#include "Tracy.hpp"
#include "asset_registry/asset_registry.hpp"
#include "renderer/renderer.hpp"
#include "renderer/standard_material.hpp"
#include "rx/core/log.h"
#include "rx/core/string.h"

#define LOG_MISSING_ATTRIBUTE(attribute_name) logger->error("No attribute %s in primitive, aborting", (attribute_name))

namespace sanity::editor::import {
    RX_LOG("SceneImporter", logger);

    constexpr const char* POSITION_ATTRIBUTE_NAME = "POSITION";
    constexpr const char* NORMAL_ATTRIBUTE_NAME = "NORMAL";
    constexpr const char* TEXCOORD_ATTRIBUTE_NAME = "TEXCOORD_0";

    constexpr const char* BASE_COLOR_TEXTURE_PARAM_NAME = "baseColorTexture";
    constexpr const char* METALLIC_ROUGHNESS_TEXTURE_PARAM_NAME = "metallicRoughnessTexture";
    constexpr const char* NORMAL_TEXTURE_PARAM_NAME = "normalTexture";

    Rx::Vector<engine::renderer::StandardMaterialHandle> import_all_materials(const tinygltf::Model& scene);

    SceneImporter::SceneImporter(engine::renderer::Renderer& renderer_in) : renderer{&renderer_in} {}

    Rx::Optional<entt::entity> SceneImporter::import_gltf_scene(const Rx::String& scene_path,
                                                                const SceneImportSettings& import_settings,
                                                                entt::registry& registry) {

        tinygltf::Model scene;
        std::string err;
        std::string warn;
        const auto success = importer.LoadASCIIFromFile(&scene, &err, &warn, scene_path.data());
        if(!warn.empty()) {
            logger->warning("Warnings when importing %s: %s", scene_path, warn.c_str());
        }
        if(!err.empty()) {
            logger->error("Errors when importing %s: %s", scene_path, err.c_str());
        }
        if(!success) {
            logger->error("Could not import scene %s", scene_path);
            return Rx::nullopt;
        }

        // How to import a scene?
        // First, load all the meshes, textures, materials, and other primitives

        Rx::Vector<engine::renderer::Mesh> meshes;
        Rx::Vector<engine::renderer::StandardMaterialHandle> materials;
        // Rx::Vector<engine::renderer::AnimationHandle> animations;
        Rx::Vector<engine::renderer::LightHandle> lights;

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

        // Finally, return the root entity

        return {};
    }

    Rx::Vector<engine::renderer::StandardMaterialHandle> SceneImporter::import_all_materials(const tinygltf::Model& scene,
                                                                                             ID3D12GraphicsCommandList4* cmds) {
        ZoneScoped;

        Rx::Vector<engine::renderer::StandardMaterialHandle> materials;

        for(const auto& material : scene.materials) {
            logger->info("Importing material %s", material.name.c_str());

            engine::renderer::StandardMaterial sanity_material{};

            // Extract the texture indices

            int base_color_texture_idx{-1};
            int metallic_roughness_texture_idx{-1};
            int normal_texture_idx{-1};

            for(const auto& [param_name, param] : material.values) {
                logger->verbose("Value %s has texture index %d", param_name.c_str(), param.TextureIndex());
                if(param_name == BASE_COLOR_TEXTURE_PARAM_NAME) {
                    base_color_texture_idx = param.TextureIndex();

                } else if(param_name == METALLIC_ROUGHNESS_TEXTURE_PARAM_NAME) {
                    metallic_roughness_texture_idx = param.TextureIndex();
                }
            }

            for(const auto& [param_name, param] : material.additionalValues) {
                logger->verbose("Additional value %s has texture index %d", param_name.c_str(), param.TextureIndex());
                if(param_name == NORMAL_TEXTURE_PARAM_NAME) {
                    normal_texture_idx = param.TextureIndex();
                }
            }

            // Validate

            if(base_color_texture_idx == -1) {
                logger->error("Parameter %s on material %s isn't a valid texture", BASE_COLOR_TEXTURE_PARAM_NAME, material.name.c_str());
                continue;
            }
            if(metallic_roughness_texture_idx == -1) {
                logger->error("Parameter %s on material %s is not a valid texture",
                              METALLIC_ROUGHNESS_TEXTURE_PARAM_NAME,
                              material.name.c_str());
                continue;
            }
            if(normal_texture_idx == -1) {
                logger->error("Parameter %s on material %s is not a valid texture", NORMAL_TEXTURE_PARAM_NAME, material.name.c_str());
                continue;
            }

            // Load the textures

            if(const auto handle = upload_texture_from_gltf(static_cast<Uint32>(base_color_texture_idx), scene, cmds); handle.has_value()) {
                sanity_material.base_color = *handle;
            }

            if(const auto handle = upload_texture_from_gltf(static_cast<Uint32>(metallic_roughness_texture_idx), scene, cmds);
               handle.has_value()) {
                sanity_material.metallic_roughness = *handle;
            }
            if(const auto handle = upload_texture_from_gltf(static_cast<Uint32>(normal_texture_idx), scene, cmds); handle.has_value()) {
                sanity_material.normal = *handle;
            }

            // Allocate material on GPU

            const auto handle = renderer->allocate_standard_material(sanity_material);
            materials.push_back(handle);
        }

        return materials;
    }

    Rx::Optional<engine::renderer::TextureHandle> SceneImporter::upload_texture_from_gltf(const Uint32 texture_idx,
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
            logger->error("Texture %s has an invalid source", texture.name.c_str());
            return Rx::nullopt;
        }

        // We only support three- or four-channel textures, with eight bits per chanel
        const auto& source_image = scene.images[texture.source];
        if(source_image.component != 3 && source_image.component != 4) {
            logger->error("Source image %s does not have either 3 or four components", source_image.name.c_str());
            return Rx::nullopt;
        }
        if(source_image.bits != 8) {
            logger->error("Source image does not have eight bits per component. Unable to load");
            return Rx::nullopt;
        }

        const void* image_data = &source_image.image.at(0);
        if(source_image.component == 3) {
            // We have to pad out the data because GPUs don't like multiples of 3
            padding_buffer = RX_SYSTEM_ALLOCATOR.reallocate(padding_buffer,
                                                            static_cast<std::size_t>(source_image.width) * source_image.height * 4);

            std::size_t read_idx{0};
            Size write_idx{0};

            for(Uint32 pixel_idx = 0; pixel_idx < source_image.width * source_image.height; pixel_idx++) {
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

    Rx::Vector<SceneImporter::ImportedMesh> SceneImporter::import_all_meshes(const tinygltf::Model& scene, ID3D12GraphicsCommandList4* cmds) {

        for(const auto& mesh : scene.meshes) {
            logger->info("Importing mesh %s", mesh.name.c_str());

            for(Uint32 primitive_idx = 0; primitive_idx < mesh.primitives.size(); primitive_idx++) {
                logger->info("Importing primitive %d", primitive_idx);

                const auto& primitive = mesh.primitives[primitive_idx];

                const auto& primitive_data = get_data_from_primitive(primitive, scene);
                if(!primitive_data) {
                    logger->error("Could not read data for primitive %d in mesh %s", primitive_idx, mesh.name.c_str());
                    continue;
                }
            }
        }
    }

    template <typename DataType>
    [[nodiscard]] const DataType* get_pointer_to_accessor_data(int accessor_idx, const tinygltf::Model& scene);

    Rx::Optional<SceneImporter::PrimitiveData> SceneImporter::get_data_from_primitive(const tinygltf::Primitive& primitive,
                                                                                      const tinygltf::Model& scene) {
        PrimitiveData data;

        data.indices = get_indices_from_primitive(primitive, scene);

        data.vertices = get_vertices_from_primitive(primitive, scene);

        return data;
    }

    Rx::Vector<Uint32> SceneImporter::get_indices_from_primitive(const tinygltf::Primitive& primitive, const tinygltf::Model& scene) {
        const auto* index_read_ptr = get_pointer_to_accessor_data<Uint32>(primitive.indices, scene);

        Rx::Vector<Uint32> indices;

        const auto& index_accessor = scene.accessors[primitive.indices];
        indices.reserve(index_accessor.count);
        for(Uint32 i = 0; i < index_accessor.count; i++) {
            indices.push_back(*index_read_ptr);
            index_read_ptr++;
        }

        return indices;
    }

    Rx::Vector<StandardVertex> SceneImporter::get_vertices_from_primitive(const tinygltf::Primitive& primitive,
                                                                          const tinygltf::Model& scene) {
        const auto& positions_itr = primitive.attributes.find(POSITION_ATTRIBUTE_NAME);
        if(positions_itr == primitive.attributes.end()) {
            LOG_MISSING_ATTRIBUTE(POSITION_ATTRIBUTE_NAME);

            return {};
        }

        const auto* position_read_ptr = get_pointer_to_accessor_data<Vec3f>(positions_itr->second, scene);
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

            return get_pointer_to_accessor_data<Vec3f>(normals_itr->second, scene);
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

            return get_pointer_to_accessor_data<Vec2f>(texcoord_itr->second, scene);
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

    template <typename DataType>
    [[nodiscard]] const DataType* get_pointer_to_accessor_data(const int accessor_idx, const tinygltf::Model& scene) {
        if(accessor_idx < 0) {
            logger->error("Accessor index is less than 0, aborting");
            return nullptr;
        }

        const auto& accessor = scene.accessors[accessor_idx];
        if(accessor.bufferView < 0) {
            logger->error("Buffer view index is less than zero, aborting");
            return nullptr;
        }

        const auto& buffer_view = scene.bufferViews[accessor.bufferView];
        if(buffer_view.target < 0) {
            logger->error("Buffer index is less than zero, aborting");
            return nullptr;
        }

        const auto& buffer = scene.buffers[buffer_view.target];
        return reinterpret_cast<const DataType*>(buffer.data.data() + buffer_view.byteStride);
    }

} // namespace sanity::editor::import
