#include "scene_importer.hpp"

#define TINYGLTF_IMPLEMENTATION

#include <sanity_engine.hpp>

#include "Tracy.hpp"
#include "asset_registry/asset_registry.hpp"
#include "renderer/renderer.hpp"
#include "renderer/standard_material.hpp"
#include "rx/core/log.h"
#include "rx/core/string.h"

namespace sanity::editor::import {
    RX_LOG("SceneImporter", logger);

    constexpr const char* BASE_COLOR_TEXTURE_PARAM_NAME = "baseColorTexture";
    constexpr const char* METALLIC_ROUGHNESS_TEXTURE_PARAM_NAME = "metallicRoughnessTexture";
    constexpr const char* NORMAL_TEXTURE_PARAM_NAME = "normalTexture";

    Rx::Vector<engine::renderer::StandardMaterialHandle> import_all_materials(const tinygltf::Model& scene);

    SceneImporter::SceneImporter(engine::renderer::Renderer& renderer_in) : renderer{&renderer_in} {}

    Rx::Optional<entt::entity> SceneImporter::import_gltf_scene(const Rx::String& scene_path, entt::registry& registry) {
        const auto meta_file_path = Rx::String::format("%s.meta", scene_path);
        const auto metadata = AssetRegistry::get_meta_for_asset<SceneImportSettings>(meta_file_path);

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
        //
        // Rx::Vector<engine::renderer::MeshHandle> meshes;
        Rx::Vector<engine::renderer::StandardMaterialHandle> materials;
        // Rx::Vector<engine::renderer::AnimationHandle> animations;
        Rx::Vector<engine::renderer::LightHandle> lights;

        auto& renderer = engine::g_engine->get_renderer();
        auto& backend = renderer.get_render_backend();
        auto cmds = backend.create_command_list();

        const auto& import_settings = metadata.import_settings;
        if(import_settings.import_materials) {
            materials = import_all_materials(scene, cmds);
        }

        backend.submit_command_list(Rx::Utility::move(cmds));

        // Then, walk the node hierarchy, creating an hierarchy of entt::entities

        // Finally, return the root entity

        return {};
    }

    Rx::Vector<engine::renderer::StandardMaterialHandle> SceneImporter::import_all_materials(
        const tinygltf::Model& scene, const ComPtr<ID3D12GraphicsCommandList4>& cmds) {
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

            const auto handle = renderer->allocate_standard_material(sanity_material);
            materials.push_back(handle);
        }

        return materials;
    }

    Rx::Optional<engine::renderer::TextureHandle> SceneImporter::upload_texture_from_gltf(const Uint32 texture_idx,
                                                                                       const tinygltf::Model& scene,
                                                                                       const ComPtr<ID3D12GraphicsCommandList4>& cmds) {
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
} // namespace sanity::editor::import
