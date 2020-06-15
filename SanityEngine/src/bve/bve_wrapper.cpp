#include "bve_wrapper.hpp"

#include <filesystem>

#include <entt/entity/registry.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <minitrace.h>
#include <stb_image.h>

#include "adapters/tracy.hpp"
#include "loading/shader_loading.hpp"
#include "renderer/renderer.hpp"
#include "renderer/standard_material.hpp"
#include "rhi/d3dx12.hpp"
#include "rhi/helpers.hpp"
#include "rhi/render_device.hpp"
#include "rx/core/log.h"

using namespace bve;

constexpr Uint32 THREAD_GROUP_WIDTH = 8;
constexpr Uint32 THREAD_GROUP_HEIGHT = 8;

static Uint32 to_Uint32(const BVE_Vector4<uint8_t>& bve_color) {
    Uint32 color{0};
    color |= bve_color.x;
    color |= bve_color.y << 8;
    color |= bve_color.z << 16;
    color |= bve_color.w << 24;

    return color;
}

static glm::vec2 to_glm_vec2(const BVE_Vector2<Float32>& bve_vec2) { return glm::vec2{bve_vec2.x, bve_vec2.y}; }

static glm::vec3 to_glm_vec3(const BVE_Vector3<Float32>& bve_vec3) { return glm::vec3{bve_vec3.x, bve_vec3.y, bve_vec3.z}; }

static const stbi_uc* expand_rgb8_to_rgba8(const stbi_uc* texture_data, const int width, const int height) {
    const auto num_pixels = width * height;
    const auto total_num_bytes = num_pixels * 4;
    auto* new_data = new stbi_uc[total_num_bytes];

    for(int i = 0; i < num_pixels; i++) {
        const auto src_idx = i * 3;
        const auto dst_idx = i * 4;

        new_data[dst_idx] = texture_data[src_idx];
        new_data[dst_idx + 1] = texture_data[src_idx + 1];
        new_data[dst_idx + 2] = texture_data[src_idx + 2];
        new_data[dst_idx + 3] = 0xFF;
    }

    return new_data;
}

RX_LOG("Bve", logger);

BveWrapper::BveWrapper(rhi::RenderDevice& device) {
    bve_init();

    create_texture_filter_pipeline(device);
}

bool BveWrapper::add_train_to_scene(const Rx::String& filename, entt::registry& registry, renderer::Renderer& renderer) {
    const auto train_msg = Rx::String::format("Load train %s", filename);
    ZoneScopedN(train_msg.data());

    const auto train = load_mesh_from_file(filename);
    if(!train) {
        logger->error("BVE returned absolutely nothing for train '%s'", filename);

        return false;

    } else if(train->errors.count > 0) {
        logger->error("Could not load train '%s'", filename);

        for(Uint32 i = 0; i < train->errors.count; i++) {
            const auto& error = train->errors.ptr[i];
            const auto error_data = get_printable_error(error);
            logger->error("%s", error_data.description);
        }

        return false;

    } else {
        if(train->meshes.count == 0) {
            logger->error("No meshes loaded for train '%s'", filename);
            return false;
        }

        auto train_path = std::filesystem::path{filename.data()};

        auto& device = renderer.get_render_device();

        auto& mesh_data = renderer.get_static_mesh_store();

        auto commands = device.create_command_list();
        mesh_data.bind_to_command_list(commands);

        commands->SetComputeRootSignature(bve_texture_pipeline->root_signature.Get());
        commands->SetPipelineState(bve_texture_pipeline->pso.Get());

        Rx::Vector<rhi::Mesh> train_meshes;
        train_meshes.reserve(train->meshes.count);

        mesh_data.begin_adding_meshes(commands);

        for(Uint32 i = 0; i < train->meshes.count; i++) {
            const auto& bve_mesh = train->meshes.ptr[i];

            const auto [vertices, indices] = process_vertices(bve_mesh);

            const auto entity = registry.create();

            auto& mesh_component = registry.assign<renderer::StandardRenderableComponent>(entity);

            mesh_component.mesh = mesh_data.add_mesh(vertices, indices, commands);
            train_meshes.push_back(mesh_component.mesh);

            if(bve_mesh.texture.texture_id.exists) {
                const auto* texture_name = BVE_Texture_Set_lookup(train->textures, bve_mesh.texture.texture_id.value);

                const auto texture_msg = Rx::String::format("Load texture %s", texture_name);
                ZoneScopedN(texture_msg.data());

                const auto texture_handle_maybe = renderer.get_image_handle(texture_name);
                if(texture_handle_maybe) {
                    logger->verbose("Texture %s has existing handle %d", texture_name, texture_handle_maybe->index);

                    const auto material = renderer::StandardMaterial{
                        .albedo = *texture_handle_maybe,
                        .normal_roughness = renderer.get_default_normal_roughness_texture(),
                        .specular_color_emission = renderer.get_default_specular_color_emission_texture(),
                        .noise = renderer.get_noise_texture(),
                    };

                    mesh_component.material = renderer.allocate_standard_material(material);

                } else {
                    const auto texture_path = train_path.replace_filename(texture_name).string();

                    int width, height, num_channels;
                    const auto* texture_data = stbi_load(texture_path.c_str(), &width, &height, &num_channels, 0);
                    if(texture_data == nullptr) {
                        logger->error("Could not load texture %s", texture_name);

                    } else {
                        if(num_channels == 3) {
                            texture_data = expand_rgb8_to_rgba8(texture_data, width, height);
                        }

                        auto create_info = rhi::ImageCreateInfo{.name = Rx::String::format("Scratch Texture %s", texture_name),
                                                                .usage = rhi::ImageUsage::SampledImage,
                                                                .format = rhi::ImageFormat::Rgba8,
                                                                .width = static_cast<Uint32>(width),
                                                                .height = static_cast<Uint32>(height),
                                                                .depth = 1};
                        const auto scratch_texture_handle = renderer.create_image(create_info, texture_data, commands);
                        const auto& scratch_texture = renderer.get_image(scratch_texture_handle);

                        // Create a second image as the real image
                        create_info.name = texture_name;
                        const auto texture_handle = renderer.create_image(create_info);
                        const auto& texture = renderer.get_image(texture_handle);

                        auto bind_group_builder = create_texture_processor_bind_group_builder(device);

                        bind_group_builder->set_image("input_texture", scratch_texture);
                        bind_group_builder->set_image("output_texture", texture);

                        auto bind_group = bind_group_builder->build();
                        bind_group->bind_to_compute_signature(commands);

                        const auto workgroup_width = (width / THREAD_GROUP_WIDTH) + 1;
                        const auto workgroup_height = (height / THREAD_GROUP_HEIGHT) + 1;

                        commands->Dispatch(workgroup_width, workgroup_height, 1);

                        renderer.schedule_texture_destruction(scratch_texture_handle);

                        logger->verbose("Newly loaded image %s has handle %d", texture_name, texture_handle.index);

                        const auto material = renderer::StandardMaterial{
                            .albedo = texture_handle,
                            .noise = renderer.get_noise_texture(),
                        };

                        mesh_component.material = renderer.allocate_standard_material(material);

                        delete[] texture_data;
                    }
                }

                bve_delete_string(const_cast<char*>(texture_name));
            }
        }

        mesh_data.end_adding_meshes(commands);

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

        const auto ray_mesh = renderer.create_raytracing_geometry(vertex_buffer, index_buffer, train_meshes, commands);

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

        device.submit_command_list(std::move(commands));

        renderer.add_raytracing_objects_to_scene(Rx::Array{rhi::RaytracingObject{.geometry_handle = ray_mesh}});

        logger->info("Loaded file %s", filename);

        return true;
    }
}

Rx::Ptr<rhi::BindGroupBuilder> BveWrapper::create_texture_processor_bind_group_builder(rhi::RenderDevice& device) {
    auto [cpu_handle, gpu_handle] = device.allocate_descriptor_table(2);
    const auto descriptor_size = device.get_shader_resource_descriptor_size();

    const Rx::Map<Rx::String, rhi::DescriptorTableDescriptorDescription>
        descriptors = Rx::Array{Rx::Pair{"input_texture",
                                         rhi::DescriptorTableDescriptorDescription{.type = rhi::DescriptorType::ShaderResource,
                                                                                   .handle = cpu_handle}},
                                Rx::Pair{"output_texture",
                                         rhi::DescriptorTableDescriptorDescription{.type = rhi::DescriptorType::UnorderedAccess,
                                                                                   .handle = cpu_handle.Offset(descriptor_size)}}};

    const Rx::Map<Uint32, D3D12_GPU_DESCRIPTOR_HANDLE> tables = Rx::Array{Rx::Pair{0, gpu_handle}};

    return device.create_bind_group_builder({}, descriptors, tables);
}

void BveWrapper::create_texture_filter_pipeline(rhi::RenderDevice& device) {
    ZoneScopedN("create_bve_texture_alpha_pipeline");

    Rx::Vector<CD3DX12_ROOT_PARAMETER> root_params{1};

    Rx::Vector<CD3DX12_DESCRIPTOR_RANGE> ranges{2};
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

    root_params[0].InitAsDescriptorTable(static_cast<UINT>(ranges.size()), ranges.data());

    const auto sig_desc = D3D12_ROOT_SIGNATURE_DESC{.NumParameters = static_cast<UINT>(root_params.size()),
                                                    .pParameters = root_params.data()};

    const auto root_sig = device.compile_root_signature(sig_desc);

    const auto compute_shader = load_shader("make_transparent_texture.compute");
    bve_texture_pipeline = device.create_compute_pipeline_state(compute_shader, root_sig);
}

BVE_User_Error_Data BveWrapper::get_printable_error(const BVE_Mesh_Error& error) { return BVE_Mesh_Error_to_data(&error); }

BveMeshHandle BveWrapper::load_mesh_from_file(const Rx::String& filename) {
    auto* mesh = bve_load_mesh_from_file(filename.data());

    if(mesh == nullptr) {
        logger->error("BVE failed to load anything for mesh '%s'", filename);
    }

    return BveMeshHandle{mesh, bve_delete_loaded_static_mesh};
}

std::pair<Rx::Vector<StandardVertex>, Rx::Vector<Uint32>> BveWrapper::process_vertices(const BVE_Mesh& mesh) const {
    RX_ASSERT(mesh.indices.count % 3 == 0, "Index count must be a multiple of three");

    const auto& bve_vertices = mesh.vertices;
    auto vertices = Rx::Vector<StandardVertex>{};
    vertices.reserve(bve_vertices.count);

    for(Uint32 i = 0; i < bve_vertices.count; i++) {
        const auto& bve_vertex = bve_vertices.ptr[i];
        vertices.push_back(StandardVertex{.position = to_glm_vec3(bve_vertex.position),
                                          .normal = {bve_vertex.normal.x, bve_vertex.normal.y, -bve_vertex.normal.z},
                                          .color = to_Uint32(bve_vertex.color),
                                          .texcoord = to_glm_vec2(bve_vertex.coord)});
    }

    const auto& bve_indices = mesh.indices;
    auto indices = Rx::Vector<Uint32>{};
    indices.reserve(bve_indices.count * 2); // worst-case

    for(Uint32 i = 0; i < bve_indices.count; i += 3) {
        const auto i0 = static_cast<Uint32>(bve_indices.ptr[i]);
        const auto i1 = static_cast<Uint32>(bve_indices.ptr[i + 1]);
        const auto i2 = static_cast<Uint32>(bve_indices.ptr[i + 2]);

        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i2);

        const auto v0_double_sided = bve_vertices.ptr[i0].double_sided;
        const auto v1_double_sided = bve_vertices.ptr[i1].double_sided;
        const auto v2_double_sided = bve_vertices.ptr[i2].double_sided;

        if(v0_double_sided || v1_double_sided || v2_double_sided) {
            indices.push_back(i2);
            indices.push_back(i1);
            indices.push_back(i0);
        }
    }

    return {vertices, indices};
}
