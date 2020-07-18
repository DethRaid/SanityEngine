#include "mesh_loading.hpp"

#include <filesystem>

#include <math.h>

#include "Tracy.hpp"
#include "TracyD3D12.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "image_loading.hpp"
#include "renderer/renderer.hpp"
#include "rhi/render_device.hpp"
#include "rx/core/log.h"
#include "rx/core/string.h"

RX_LOG("MeshLoading", logger);

Rx::Optional<renderer::MeshObject> import_mesh(const Rx::String& filepath,
                                               com_ptr<ID3D12GraphicsCommandList4> commands,
                                               renderer::Renderer& renderer) {
    ZoneScoped;
    static Assimp::Importer importer;

    TracyD3D12Zone(renderer::RenderDevice::tracy_context, commands.get(), "load_mesh");
    PIXScopedEvent(commands.get(), PIX_COLOR_DEFAULT, "load_mesh");

    const auto* scene = importer.ReadFile(filepath.data(), aiProcess_MakeLeftHanded | aiProcessPreset_TargetRealtime_MaxQuality);
    if(scene == nullptr) {
        logger->error("Could not load mesh %s", filepath);
        return Rx::nullopt;
    }

    // Assume there's one mesh and hope I'm right
    const auto* node = scene->mRootNode->mChildren[0];
    RX_ASSERT(node->mNumMeshes == 1, "Sanity Engine only supports one mesh per file");

    auto& mesh_data = renderer.get_static_mesh_store();

    // Get the mesh at this index
    const auto ass_mesh_handle = node->mMeshes[0];
    const auto* ass_mesh = scene->mMeshes[ass_mesh_handle];

    Vec2f x_min_max{1000, -1000};
    Vec2f y_min_max{1000, -1000};
    Vec2f z_min_max{1000, -1000};

    // Convert it to our vertex format
    Rx::Vector<StandardVertex> vertices;
    vertices.reserve(ass_mesh->mNumVertices);

    for(Uint32 vert_idx = 0; vert_idx < ass_mesh->mNumVertices; vert_idx++) {
        const auto& position = ass_mesh->mVertices[vert_idx];
        const auto& normal = ass_mesh->mNormals[vert_idx];
        // const auto& color = mesh->mColors[0][vert_idx];
        const auto& texcoord = ass_mesh->mTextureCoords[0][vert_idx];

        vertices.push_back(StandardVertex{.position = {position.x, position.y, position.z},
                                          .normal = {normal.x, normal.y, normal.z},
                                          .texcoord = {texcoord.x, texcoord.y}});

        x_min_max.x = fmin(x_min_max.x, position.x);
        x_min_max.y = fmax(x_min_max.y, position.x);
        y_min_max.x = fmin(y_min_max.x, position.y);
        y_min_max.y = fmax(y_min_max.y, position.y);
        z_min_max.x = fmin(z_min_max.x, position.z);
        z_min_max.y = fmax(z_min_max.y, position.z);
    }

    Rx::Vector<Uint32> indices;
    indices.reserve(ass_mesh->mNumFaces * 3);
    for(Uint32 face_idx = 0; face_idx < ass_mesh->mNumFaces; face_idx++) {
        const auto& face = ass_mesh->mFaces[face_idx];
        indices.push_back(face.mIndices[0]);
        indices.push_back(face.mIndices[1]);
        indices.push_back(face.mIndices[2]);
    }

    mesh_data.begin_adding_meshes(commands.get());

    const auto mesh = mesh_data.add_mesh(vertices, indices, commands.get());

    mesh_data.end_adding_meshes(commands.get());

    auto material = renderer::StandardMaterial{};
    material.noise = renderer.get_noise_texture();

    const auto* ass_material = scene->mMaterials[ass_mesh->mMaterialIndex];

    // TODO: Useful logic to select between material formats
    aiString ass_texture_path;
    auto result = ass_material->GetTexture(aiTextureType_DIFFUSE, 0, &ass_texture_path);
    if(result == aiReturn_SUCCESS) {
        // Load texture into Sanity Engine and set on material

        if(const auto existing_image_handle = renderer.get_image_handle(ass_texture_path.C_Str())) {
            material.albedo = *existing_image_handle;

        } else {
            auto path = std::filesystem::path{filepath.data()};
            const auto texture_path = path.replace_filename(ass_texture_path.C_Str());

            Uint32 width, height;
            Rx::Vector<Uint8> pixels;
            const auto was_image_loaded = load_image(texture_path.string().c_str(), width, height, pixels);
            if(!was_image_loaded) {
                logger->warning("Could not load texture %s", texture_path.string().c_str());

                material.albedo = renderer.get_pink_texture();

            } else {
                const auto create_info = renderer::ImageCreateInfo{.name = ass_texture_path.C_Str(),
                                                                   .usage = renderer::ImageUsage::SampledImage,
                                                                   .width = width,
                                                                   .height = height};

                const auto image_handle = renderer.create_image(create_info, pixels.data(), commands);
                material.albedo = image_handle;
            }
        }
    } else {
        // Get the material base color. Create a renderer texture with this color, set that texture as the albedo
        // If there's no material base color, use a pure white texture
        logger->warning("No diffuse texture in mesh %s - please code up a fallback");
    }

    material.normal_roughness = renderer.get_default_normal_roughness_texture();
    material.specular_color_emission = renderer.get_default_specular_color_emission_texture();

    // TODO: Render an image of the object to display in editor previews - use the min/max of the object from the mesh

    return renderer::MeshObject{.mesh = mesh,
                                .bounds = {.x_min = x_min_max.x,
                                           .x_max = x_min_max.y,
                                           .y_min = y_min_max.x,
                                           .y_max = y_min_max.y,
                                           .z_min = z_min_max.x,
                                           .z_max = z_min_max.y}};
}
