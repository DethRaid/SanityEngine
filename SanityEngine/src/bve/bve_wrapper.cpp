#include "bve_wrapper.hpp"

#include <filesystem>

#include <entt/entity/registry.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <minitrace.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <stb_image.h>

#include "../core/ensure.hpp"
#include "../loading/shader_loading.hpp"
#include "../renderer/renderer.hpp"
#include "../rhi/d3dx12.hpp"
#include "../rhi/render_device.hpp"
#include "train_components.hpp"

using namespace bve;

constexpr uint32_t THREAD_GROUP_WIDTH = 8;
constexpr uint32_t THREAD_GROUP_HEIGHT = 8;

struct BveMaterial {
    renderer::ImageHandle color_texture;
};

static uint32_t to_uint32_t(const BVE_Vector4<uint8_t>& bve_color) {
    uint32_t color{0};
    color |= bve_color.x;
    color |= bve_color.y << 8;
    color |= bve_color.z << 16;
    color |= bve_color.w << 24;

    return color;
}

static glm::vec2 to_glm_vec2(const BVE_Vector2<float>& bve_vec2) { return glm::vec2{bve_vec2.x, bve_vec2.y}; }

static glm::vec3 to_glm_vec3(const BVE_Vector3<float>& bve_vec3) { return glm::vec3{bve_vec3.x, bve_vec3.y, bve_vec3.z}; }

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

std::shared_ptr<spdlog::logger> BveWrapper::logger{spdlog::stdout_color_st("Bve")};

BveWrapper::BveWrapper(rhi::RenderDevice& device) {
    bve_init();

    create_texture_filter_pipeline(device);
}

bool BveWrapper::add_train_to_scene(const std::string& filename, entt::registry& registry, renderer::Renderer& renderer) {
    const auto train_msg = fmt::format("Load train {}", filename);
    MTR_SCOPE("SanityEngine", train_msg.c_str());

    const auto train = load_mesh_from_file(filename);
    if(!train) {
        logger->error("BVE returned absolutely nothing for train '{}'", filename);

    } else if(train->errors.count > 0) {
        logger->error("Could not load train '{}'", filename);

        for(uint32_t i = 0; i < train->errors.count; i++) {
            const auto& error = train->errors.ptr[i];
            const auto error_data = get_printable_error(error);
            logger->error("{}", error_data.description);
        }

        return false;

    } else {
        if(train->meshes.count == 0) {
            logger->error("No meshes loaded for train '{}'", filename);
            return false;
        }

        const auto train_entity = registry.create();
        auto& train_component = registry.assign<TrainComponent>(train_entity);

        auto train_path = std::filesystem::path{filename};

        auto& device = renderer.get_render_device();

        auto commands = device.create_compute_command_list();
        commands->bind_mesh_data(renderer.get_static_mesh_store());

        commands->set_pipeline_state(*bve_texture_pipeline);

        std::vector<rhi::Mesh> train_meshes;
        train_meshes.reserve(train->meshes.count);

        for(uint32_t i = 0; i < train->meshes.count; i++) {
            const auto& bve_mesh = train->meshes.ptr[i];

            const auto [vertices, indices] = process_vertices(bve_mesh);

            const auto entity = registry.create();

            auto& mesh_component = registry.assign<renderer::StaticMeshRenderableComponent>(entity);

            mesh_component.mesh = renderer.create_static_mesh(vertices, indices, *commands);
            train_meshes.push_back(mesh_component.mesh);

            if(bve_mesh.texture.texture_id.exists) {
                const auto* texture_name = BVE_Texture_Set_lookup(train->textures, bve_mesh.texture.texture_id.value);

                const auto texture_msg = fmt::format("Load texture {}", texture_name);
                MTR_SCOPE("SanityEngine", texture_msg.c_str());

                const auto texture_handle_maybe = renderer.get_image_handle(texture_name);
                if(texture_handle_maybe) {
                    logger->debug("Texture {} has existing handle {}", texture_name, texture_handle_maybe->idx);

                    auto& material_data = renderer.get_material_data_buffer();
                    const auto material_handle = material_data.get_next_free_material<BveMaterial>();
                    auto& material = material_data.at<BveMaterial>(material_handle);
                    material.color_texture = *texture_handle_maybe;

                    mesh_component.material = material_handle;

                } else {
                    const auto texture_path = train_path.replace_filename(texture_name).string();

                    int width, height, num_channels;
                    const auto* texture_data = stbi_load(texture_path.c_str(), &width, &height, &num_channels, 0);
                    if(texture_data == nullptr) {
                        logger->error("Could not load texture {}", texture_name);

                    } else {
                        if(num_channels == 3) {
                            texture_data = expand_rgb8_to_rgba8(texture_data, width, height);
                        }

                        auto create_info = rhi::ImageCreateInfo{.name = fmt::format("Scratch Texture {}", texture_name),
                                                                .usage = rhi::ImageUsage::SampledImage,
                                                                .format = rhi::ImageFormat::Rgba8,
                                                                .width = static_cast<uint32_t>(width),
                                                                .height = static_cast<uint32_t>(height),
                                                                .depth = 1};
                        const auto staging_texture_handle = renderer.create_image(create_info);
                        const auto& staging_texture = renderer.get_image(staging_texture_handle);

                        commands->copy_data_to_image(texture_data, staging_texture);

                        // Create a second image as the real image
                        create_info.name = texture_name;
                        const auto texture_handle = renderer.create_image(create_info);
                        const auto& texture = renderer.get_image(texture_handle);

                        auto bind_group_builder = create_texture_processor_bind_group_builder(device);
                        ;

                        bind_group_builder->set_image("input_texture", staging_texture);
                        bind_group_builder->set_image("output_texture", texture);

                        auto bind_group = bind_group_builder->build();
                        commands->bind_compute_resources(*bind_group);

                        const auto workgroup_width = (width / THREAD_GROUP_WIDTH) + 1;
                        const auto workgroup_height = (height / THREAD_GROUP_HEIGHT) + 1;

                        commands->dispatch(workgroup_width, workgroup_height);

                        renderer.schedule_texture_destruction(staging_texture_handle);

                        logger->debug("Newly loaded image {} has handle {}", texture_name, texture_handle.idx);

                        auto& material_data = renderer.get_material_data_buffer();
                        const auto material_handle = material_data.get_next_free_material<BveMaterial>();
                        auto& material = material_data.at<BveMaterial>(material_handle);
                        material.color_texture = texture_handle;

                        mesh_component.material = material_handle;

                        delete[] texture_data;
                    }
                }

                bve_delete_string(const_cast<char*>(texture_name));
            }
        }

        train_component.raytracing_mesh = commands->build_acceleration_structure_for_meshes(train_meshes);

        device.submit_command_list(std::move(commands));

        renderer.add_raytracing_objects_to_scene({rhi::RaytracingObject{.mesh = &train_component.raytracing_mesh}});

        logger->info("Loaded file {}", filename);

        return true;
    }
}

std::unique_ptr<rhi::BindGroupBuilder> BveWrapper::create_texture_processor_bind_group_builder(rhi::RenderDevice& device) {
    auto [cpu_handle, gpu_handle] = device.allocate_descriptor_table(2);
    const auto descriptor_size = device.get_shader_resource_descriptor_size();

    const std::unordered_map<std::string, rhi::DescriptorTableDescriptorDescription> descriptors =
        {{"input_texture", {.type = rhi::DescriptorType::ShaderResource, .handle = cpu_handle}},
         {"output_texture", {.type = rhi::DescriptorType::UnorderedAccess, .handle = cpu_handle.Offset(descriptor_size)}}};

    const std::unordered_map<uint32_t, D3D12_GPU_DESCRIPTOR_HANDLE> tables = {{0, gpu_handle}};

    return device.create_bind_group_builder({}, descriptors, tables);
}

void BveWrapper::create_texture_filter_pipeline(rhi::RenderDevice& device) {
    MTR_SCOPE("Renderer", "create_bve_texture_alpha_pipeline");

    std::vector<CD3DX12_ROOT_PARAMETER> root_params{1};

    std::vector<CD3DX12_DESCRIPTOR_RANGE> ranges{2};
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

    root_params[0].InitAsDescriptorTable(static_cast<UINT>(ranges.size()), ranges.data());

    const auto sig_desc = D3D12_ROOT_SIGNATURE_DESC{.NumParameters = static_cast<UINT>(root_params.size()),
                                                    .pParameters = root_params.data()};

    const auto root_sig = device.compile_root_signature(sig_desc);

    const auto compute_shader = load_shader("data/shaders/make_transparent_texture.compute");
    bve_texture_pipeline = device.create_compute_pipeline_state(compute_shader, root_sig);
}

BVE_User_Error_Data BveWrapper::get_printable_error(const BVE_Mesh_Error& error) { return BVE_Mesh_Error_to_data(&error); }

BveMeshHandle BveWrapper::load_mesh_from_file(const std::string& filename) {
    auto* mesh = bve_load_mesh_from_file(filename.c_str());

    if(mesh == nullptr) {
        logger->error("BVE failed to load anything for mesh '{}'", filename);
    }

    return BveMeshHandle{mesh, bve_delete_loaded_static_mesh};
}

std::pair<std::vector<BveVertex>, std::vector<uint32_t>> BveWrapper::process_vertices(const BVE_Mesh& mesh) const {
    ENSURE(mesh.indices.count % 3 == 0, "Index count must be a multiple of three");

    const auto& bve_vertices = mesh.vertices;
    auto vertices = std::vector<BveVertex>{};
    vertices.reserve(bve_vertices.count);

    std::transform(bve_vertices.ptr, bve_vertices.ptr + bve_vertices.count, std::back_inserter(vertices), [](const BVE_Vertex& bve_vertex) {
        return BveVertex{.position = to_glm_vec3(bve_vertex.position),
                         .normal = to_glm_vec3(bve_vertex.normal),
                         .color = to_uint32_t(bve_vertex.color),
                         .texcoord = to_glm_vec2(bve_vertex.coord)};
    });

    const auto& bve_indices = mesh.indices;
    auto indices = std::vector<uint32_t>{};
    indices.reserve(bve_indices.count * 2); // worst-case

    for(uint32_t i = 0; i < bve_indices.count; i += 3) {
        const auto i0 = static_cast<uint32_t>(bve_indices.ptr[i]);
        const auto i1 = static_cast<uint32_t>(bve_indices.ptr[i + 1]);
        const auto i2 = static_cast<uint32_t>(bve_indices.ptr[i + 2]);

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
