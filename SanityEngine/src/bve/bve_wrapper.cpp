#include "bve_wrapper.hpp"

#include <filesystem>

#include <entt/entity/registry.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <minitrace.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <stb_image.h>

#include "../core/ensure.hpp"
#include "../renderer/renderer.hpp"

using namespace bve;

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

    for(uint32_t i = 0; i < num_pixels; i++) {
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

BveWrapper::BveWrapper() { bve_init(); }

bool BveWrapper::add_train_to_scene(const std::string& filename, entt::registry& registry, renderer::Renderer& renderer) {
    const auto train_msg = fmt::format("Load train {}", filename);
    MTR_SCOPE("SanityEngine", train_msg.c_str());

    const auto train = load_mesh_from_file(filename);
    if(train->errors.count > 0) {
        logger->error("Could not load train '{}'", filename);

        for(uint32_t i = 0; i < train->errors.count; i++) {
            const auto& error = train->errors.ptr[i];
            const auto error_data = get_printable_error(error);
            logger->error("{}", error_data.description);
        }

    } else {
        if(train->meshes.count == 0) {
            logger->error("No meshes loaded for train '{}'", filename);
            return false;
        }

        auto train_path = std::filesystem::path{filename};

        for(uint32_t i = 0; i < train->meshes.count; i++) {
            const auto& bve_mesh = train->meshes.ptr[i];

            const auto [vertices, indices] = process_vertices(bve_mesh);

            auto mesh = renderer.create_static_mesh(vertices, indices);

            if(bve_mesh.texture.texture_id.exists) {
                const auto* texture_name = bve::BVE_Texture_Set_lookup(train->textures, bve_mesh.texture.texture_id.value);

                const auto texture_msg = fmt::format("Load texture {}", texture_name);
                MTR_SCOPE("SanityEngine", texture_msg.c_str());

                const auto texture_handle_maybe = renderer.get_image_handle(texture_name);
                if(texture_handle_maybe) {
                    logger->debug("Texture {} has existing handle {}", texture_name, texture_handle_maybe->idx);
                    auto& material_data = renderer.get_material_data_buffer();
                    const auto material_handle = material_data.get_next_free_material<BveMaterial>();
                    auto& material = material_data.at<BveMaterial>(material_handle);
                    material.color_texture = *texture_handle_maybe;

                    mesh.material = material_handle;

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

                        const auto create_info = rhi::ImageCreateInfo{.name = texture_name,
                                                                      .usage = rhi::ImageUsage::SampledImage,
                                                                      .format = rhi::ImageFormat::Rgba8,
                                                                      .width = static_cast<uint32_t>(width),
                                                                      .height = static_cast<uint32_t>(height),
                                                                      .depth = 1};
                        const auto texture_handle = renderer.create_image(create_info, texture_data);

                        logger->debug("Newly loaded image {} has handle {}", texture_name, texture_handle.idx);

                        auto& material_data = renderer.get_material_data_buffer();
                        const auto material_handle = material_data.get_next_free_material<BveMaterial>();
                        auto& material = material_data.at<BveMaterial>(material_handle);
                        material.color_texture = texture_handle;

                        mesh.material = material_handle;

                        delete[] texture_data;
                    }
                }

                bve::bve_delete_string(const_cast<char*>(texture_name));
            }

            const auto entity = registry.create();

            registry.assign<renderer::StaticMeshRenderableComponent>(entity, std::move(mesh));
        }

        logger->info("Loaded file {}", filename);
    }
}

BVE_User_Error_Data BveWrapper::get_printable_error(const BVE_Mesh_Error& error) { return BVE_Mesh_Error_to_data(&error); }

BveMeshHandle BveWrapper::load_mesh_from_file(const std::string& filename) {
    auto* mesh = bve_load_mesh_from_file(filename.c_str());

    return BveMeshHandle{mesh, bve_delete_loaded_static_mesh};
}

std::pair<std::vector<BveVertex>, std::vector<uint32_t>> BveWrapper::process_vertices(const BVE_Mesh& mesh) {
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
