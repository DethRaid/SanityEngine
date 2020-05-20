#include "entity_loading.hpp"

#include <filesystem>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <entt/entity/registry.hpp>
#include <spdlog/sinks/stdout_color_sinks-inl.h>

#include "../core/ensure.hpp"
#include "../renderer/render_components.hpp"
#include "../renderer/renderer.hpp"
#include "../renderer/standard_material.hpp"
#include "../rhi/mesh_data_store.hpp"
#include "../rhi/raytracing_structs.hpp"
#include "../rhi/render_device.hpp"
#include "image_loading.hpp"

static Assimp::Importer importer;

static std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("Entity Loading");

bool load_static_mesh(const std::string& filename, entt::registry& registry, renderer::Renderer& renderer) {
    const auto* scene = importer.ReadFile(filename, aiProcess_MakeLeftHanded | aiProcessPreset_TargetRealtime_MaxQuality);

    if(scene == nullptr) {
        logger->error("Could not load {}: {}", filename, importer.GetErrorString());
        return false;
    }

    auto& device = renderer.get_render_device();
    auto command_list = device.create_compute_command_list();

    std::unordered_map<uint32_t, renderer::MaterialHandle> materials;

    std::vector<rhi::Mesh> meshes;
    std::vector<rhi::RaytracingObject> raytracing_objects;

    command_list->bind_mesh_data(renderer.get_static_mesh_store());

    // Initial revision: import the first child node and hope it's fine
    const auto* node = scene->mRootNode->mChildren[0];
    ENSURE(node->mNumMeshes == 1, "Sanity Engine currently only supports one mesh");
    // Get the mesh at this index
    const auto ass_mesh_handle = node->mMeshes[0];
    const auto* ass_mesh = scene->mMeshes[ass_mesh_handle];

    // Convert it to our vertex format
    std::vector<StandardVertex> vertices;
    vertices.reserve(ass_mesh->mNumVertices);

    for(uint32_t vert_idx = 0; vert_idx < ass_mesh->mNumVertices; vert_idx++) {
        const auto& position = ass_mesh->mVertices[vert_idx];
        const auto& normal = ass_mesh->mNormals[vert_idx];
        // const auto& tangent = mesh->mTangents[vert_idx];
        // const auto& color = mesh->mColors[0][vert_idx];
        const auto& texcoord = ass_mesh->mTextureCoords[0][vert_idx];

        vertices.push_back(StandardVertex{.position = {position.x, position.y, position.z},
                                          .normal = {normal.x, normal.y, normal.z},
                                          .texcoord = {texcoord.x, texcoord.y}});
    }

    std::vector<uint32_t> indices;
    indices.reserve(ass_mesh->mNumFaces * 3);
    for(uint32_t face_idx = 0; face_idx < ass_mesh->mNumFaces; face_idx++) {
        const auto& face = ass_mesh->mFaces[face_idx];
        indices.push_back(face.mIndices[0]);
        indices.push_back(face.mIndices[1]);
        indices.push_back(face.mIndices[2]);
    }

    const auto mesh = renderer.create_static_mesh(vertices, indices, *command_list);

    const auto mesh_entity = registry.create();
    auto& mesh_renderer = registry.assign<renderer::StaticMeshRenderableComponent>(mesh_entity);
    mesh_renderer.mesh = mesh;

    meshes.push_back(mesh);

    if(const auto& mat = materials.find(ass_mesh->mMaterialIndex); mat != materials.end()) {
        mesh_renderer.material = mat->second;

    } else {
        auto& material_store = renderer.get_material_data_buffer();
        const auto material_handle = material_store.get_next_free_material<StandardMaterial>();
        auto& material = material_store.at<StandardMaterial>(material_handle);

        material.noise = renderer.get_noise_texture();

        mesh_renderer.material = material_handle;

        const auto* ass_material = scene->mMaterials[ass_mesh->mMaterialIndex];

        // TODO: Useful logic to select between material formats
        aiString ass_texture_path;
        auto result = ass_material->GetTexture(aiTextureType_DIFFUSE, 0, &ass_texture_path);
        if(result == aiReturn_SUCCESS) {
            // Load texture into Sanity Engine and set on material

            if(const auto existing_image_handle = renderer.get_image_handle(ass_texture_path.C_Str())) {
                material.albedo = *existing_image_handle;

            } else {
                auto path = std::filesystem::path{filename};
                const auto texture_path = path.replace_filename(ass_texture_path.C_Str());

                uint32_t width, height;
                std::vector<uint8_t> pixels;
                const auto was_image_loaded = load_image(texture_path.string(), width, height, pixels);
                if(!was_image_loaded) {
                    logger->warn("Could not load texture {}", texture_path.string());

                    material.albedo = renderer.get_pink_texture();

                } else {
                    const auto create_info = rhi::ImageCreateInfo{.name = ass_texture_path.C_Str(),
                                                                  .usage = rhi::ImageUsage::SampledImage,
                                                                  .width = width,
                                                                  .height = height};

                    const auto image_handle = renderer.create_image(create_info);
                    const auto& image = renderer.get_image(image_handle);
                    command_list->copy_data_to_image(pixels.data(), image);
                    material.albedo = image_handle;
                }
            }
        } else {
            // Get the material base color. Create a renderer texture with this color, set that texture as the albedo
            // If there's no material base color, use a pure white texture
        }

        material.normal_roughness = renderer.get_default_normal_roughness_texture();
        material.specular_color_emission = renderer.get_default_specular_color_emission_texture();
    }

    const auto object_entity = registry.create();
    auto& ray_mesh = registry.assign<renderer::RaytracableMeshComponent>(object_entity);
    ray_mesh.raytracing_mesh = command_list->build_acceleration_structure_for_meshes(meshes);

    renderer.add_raytracing_objects_to_scene(
        {rhi::RaytracingObject{.blas_buffer = ray_mesh.raytracing_mesh.blas_buffer.get(), .material = {mesh_renderer.material.index}}});

    device.submit_command_list(std::move(command_list));

    return true;
}
