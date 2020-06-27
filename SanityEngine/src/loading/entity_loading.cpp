#include "entity_loading.hpp"

#include <filesystem>

#include <Tracy.hpp>
#include <TracyD3D12.hpp>
#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <entt/entity/registry.hpp>
#include <rx/core/log.h>
#include <rx/core/string.h>

#include "image_loading.hpp"
#include "renderer/render_components.hpp"
#include "renderer/renderer.hpp"
#include "renderer/standard_material.hpp"
#include "rhi/helpers.hpp"
#include "rhi/mesh_data_store.hpp"
#include "rhi/raytracing_structs.hpp"
#include "rhi/render_device.hpp"

static Assimp::Importer importer;

RX_LOG("EntityLoading", logger);

bool load_static_mesh(const Rx::String& filename, SynchronizedResource<entt::registry>& registry, renderer::Renderer& renderer) {
    ZoneScoped;
    const auto* scene = importer.ReadFile(filename.data(), aiProcess_MakeLeftHanded | aiProcessPreset_TargetRealtime_MaxQuality);

    if(scene == nullptr) {
        logger->error("Could not load %s: %s", filename, importer.GetErrorString());
        return false;
    }

    auto& device = renderer.get_render_device();
    auto commands = device.create_command_list();
    commands->SetName(L"Renderer::create_raytracing_geometry");

    {
        TracyD3D12Zone(renderer::RenderDevice::tracy_context, commands.Get(), "Renderer::create_raytracing_geometry");
        PIXScopedEvent(commands.Get(), PIX_COLOR_DEFAULT, "Renderer::create_raytracing_geometry");

        Rx::Map<Uint32, renderer::StandardMaterialHandle> materials;

        Rx::Vector<renderer::Mesh> meshes;
        Rx::Vector<renderer::RaytracingObject> raytracing_objects;

        auto& mesh_data = renderer.get_static_mesh_store();

        mesh_data.bind_to_command_list(commands);

        // Initial revision: import the first child node and hope it's fine
        const auto* node = scene->mRootNode->mChildren[0];
        RX_ASSERT(node->mNumMeshes == 1, "Sanity Engine currently only supports one mesh");
        // Get the mesh at this index
        const auto ass_mesh_handle = node->mMeshes[0];
        const auto* ass_mesh = scene->mMeshes[ass_mesh_handle];

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
        }

        Rx::Vector<Uint32> indices;
        indices.reserve(ass_mesh->mNumFaces * 3);
        for(Uint32 face_idx = 0; face_idx < ass_mesh->mNumFaces; face_idx++) {
            const auto& face = ass_mesh->mFaces[face_idx];
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }

        mesh_data.begin_adding_meshes(commands);

        const auto mesh = mesh_data.add_mesh(vertices, indices, commands);

        mesh_data.end_adding_meshes(commands);

        auto locked_registry = registry.lock();
        const auto mesh_entity = locked_registry->create();
        auto& mesh_renderer = locked_registry->assign<renderer::StandardRenderableComponent>(mesh_entity);
        mesh_renderer.mesh = mesh;

        meshes.push_back(mesh);

        if(const auto* mat = materials.find(ass_mesh->mMaterialIndex)) {
            mesh_renderer.material = *mat;

        } else {
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
                    auto path = std::filesystem::path{filename.data()};
                    const auto texture_path = path.replace_filename(ass_texture_path.C_Str());

                    Uint32 width, height;
                    Rx::Vector<uint8_t> pixels;
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
            }

            material.normal_roughness = renderer.get_default_normal_roughness_texture();
            material.specular_color_emission = renderer.get_default_specular_color_emission_texture();

            mesh_renderer.material = renderer.allocate_standard_material(material);
        }

        const auto& index_buffer = mesh_data.get_index_buffer();
        const auto& vertex_buffer = *mesh_data.get_vertex_bindings()[0].buffer;

        {
            Rx::Vector<D3D12_RESOURCE_BARRIER> barriers{2};
            barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(index_buffer.resource.Get(),
                                                               D3D12_RESOURCE_STATE_INDEX_BUFFER,
                                                               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(vertex_buffer.resource.Get(),
                                                               D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                                                               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

            commands->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
        }

        const auto ray_geo_handle = renderer.create_raytracing_geometry(vertex_buffer, index_buffer, meshes, commands);

        {
            Rx::Vector<D3D12_RESOURCE_BARRIER> barriers{2};
            barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(index_buffer.resource.Get(),
                                                               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                                               D3D12_RESOURCE_STATE_INDEX_BUFFER);
            barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(vertex_buffer.resource.Get(),
                                                               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                                                               D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

            commands->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
        }

        renderer.add_raytracing_objects_to_scene(Rx::Array{renderer::RaytracingObject{.geometry_handle = ray_geo_handle, .material = {0}}});
    }

    device.submit_command_list(std::move(commands));

    return true;
}
