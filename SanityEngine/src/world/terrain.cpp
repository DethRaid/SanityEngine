#include "terrain.hpp"

#include <entt/entity/registry.hpp>
#include <ftl/atomic_counter.h>
#include <ftl/task.h>
#include <minitrace.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "../core/ensure.hpp"
#include "../loading/image_loading.hpp"
#include "../renderer/standard_material.hpp"
#include "../renderer/textures.hpp"
#include "../rhi/helpers.hpp"
#include "../rhi/render_device.hpp"
#include "../sanity_engine.hpp"

std::shared_ptr<spdlog::logger> Terrain::logger = spdlog::stdout_color_st("Terrain");

Terrain::Terrain(const uint32_t max_latitude_in,
                 const uint32_t max_longitude_in,
                 const uint32_t min_terrain_height_in,
                 const uint32_t max_terrain_height_in,
                 renderer::Renderer& renderer_in,
                 FastNoiseSIMD& noise_generator_in,
                 entt::registry& registry_in)
    : renderer{&renderer_in},
      noise_generator{&noise_generator_in},
      registry{&registry_in},
      max_latitude{max_latitude_in},
      max_longitude{max_longitude_in},
      min_terrain_height{min_terrain_height_in},
      max_terrain_height{max_terrain_height_in} {

    logger->set_level(spdlog::level::trace);

    // TODO: Make a good data structure to load the terrain material(s) at runtime
    load_terrain_textures_and_create_material();
}

void Terrain::load_terrain_around_player(const TransformComponent& player_transform) {
    MTR_SCOPE("Terrain", "load_terrain_around_player");
    const auto coords_of_tile_containing_player = get_coords_of_tile_containing_position(player_transform.position);

    // V0: load the tile the player is in and nothing else

    // V1: load the tiles in the player's frustum, plus a few on either side so it's nice and fast for the player to spin around

    // TODO: Define some maximum number of tiles that may be loaded/generated in a given frame
    // Also TODO: Generate terrain tiles in in separate threads as part of the tasking system
    if(!loaded_terrain_tiles.contains(coords_of_tile_containing_player)) {
        generate_tile(coords_of_tile_containing_player);
    }
}

float Terrain::get_terrain_height(const glm::vec2& location) {
    const auto tilecoords = get_coords_of_tile_containing_position({location.x, 0, location.y});

    const auto tile_start_location = tilecoords * TILE_SIZE;
    const auto location_within_tile = glm::uvec2{abs(round(location - glm::vec2{tile_start_location}))};

    if(const auto itr = loaded_terrain_tiles.find(tilecoords); itr != loaded_terrain_tiles.end()) {
        const auto& tile = itr->second;
        return tile.heightmap[location_within_tile.y][location_within_tile.x];

    } else {
        // Tile isn't loaded yet. Figure out how to handle this. Right now I don't want to deal with it, so I won't
        return 0;
    }
}

glm::ivec2 Terrain::get_coords_of_tile_containing_position(const glm::vec3& position) {
    return glm::ivec2{round(position.x), round(position.z)} / TILE_SIZE;
}

void Terrain::load_terrain_textures_and_create_material() {
    ftl::AtomicCounter counter{task_scheduler.get()};

    std::unique_ptr<LoadImageToGpuArgs> albedo_image_data = std::make_unique<LoadImageToGpuArgs>();
    albedo_image_data->texture_name_in = "data/textures/terrain/Ground_Forest_sfjmafua_8K_surface_ms/sfjmafua_512_Albedo.jpg";
    albedo_image_data->renderer_in = renderer;

    task_scheduler->AddTask({load_image_to_gpu, albedo_image_data.get()}, &counter);

    std::unique_ptr<LoadImageToGpuArgs> normal_roughness_image_data = std::make_unique<LoadImageToGpuArgs>();
    normal_roughness_image_data
        ->texture_name_in = "data/textures/terrain/Ground_Forest_sfjmafua_8K_surface_ms/sfjmafua_512_Normal_Roughness.jpg";
    normal_roughness_image_data->renderer_in = renderer;

    task_scheduler->AddTask({load_image_to_gpu, normal_roughness_image_data.get()}, &counter);

    auto material = renderer::StandardMaterial{};
    material.noise = renderer->get_noise_texture();

    task_scheduler->WaitForCounter(&counter, 0, true);

    if(albedo_image_data->handle_out) {
        material.albedo = *albedo_image_data->handle_out;
    } else {
        logger->error("Could not load terrain albedo texture {}", albedo_image_data->texture_name_in);
        material.albedo = renderer->get_pink_texture();
    }

    if(normal_roughness_image_data->handle_out) {
        material.normal_roughness = *normal_roughness_image_data->handle_out;
    } else {
        logger->error("Could not load terrain normal roughness texture {}", normal_roughness_image_data->texture_name_in);
        material.normal_roughness = renderer->get_default_normal_roughness_texture();
    }

    terrain_material = renderer->allocate_standard_material(material);
}

void Terrain::generate_tile(const glm::ivec2& tilecoord) {
    const auto top_left = tilecoord * TILE_SIZE;
    const auto size = glm::uvec2{TILE_SIZE};

    logger->info("Generating tile ({}, {}) with size ({}, {})", tilecoord.x, tilecoord.y, size.x, size.y);

    const auto tile_heightmap = generate_terrain_heightmap(top_left, size);

    std::vector<StandardVertex> tile_vertices;
    tile_vertices.reserve(tile_heightmap.size() * tile_heightmap[0].size());

    std::vector<uint32_t> tile_indices;
    tile_indices.reserve(tile_vertices.size() * 6);

    for(uint32_t y = 0; y < tile_heightmap.size(); y++) {
        const auto& tile_heightmap_row = tile_heightmap[y];
        for(uint32_t x = 0; x < tile_heightmap_row.size(); x++) {
            const auto height = tile_heightmap_row[x];

            const auto normal = get_normal_at_location(glm::vec2{x, y});

            tile_vertices.push_back(StandardVertex{.position = {x, height, y}, .normal = normal, .color = 0xFFFFFFFF, .texcoord = {x, y}});

            if(x < tile_heightmap_row.size() - 1 && y < tile_heightmap.size() - 1) {
                const auto width = static_cast<uint32_t>(tile_heightmap_row.size());
                const auto face_start_idx = static_cast<uint32_t>(y * width + x);

                tile_indices.push_back(face_start_idx);
                tile_indices.push_back(face_start_idx + 1);
                tile_indices.push_back(face_start_idx + width);

                tile_indices.push_back(face_start_idx + width);
                tile_indices.push_back(face_start_idx + 1);
                tile_indices.push_back(face_start_idx + width + 1);
            }
        }
    }

    auto& device = renderer->get_render_device();
    const auto commands = device.create_command_list();
    auto& meshes = renderer->get_static_mesh_store();

    meshes.begin_adding_meshes(commands);

    const auto tile_mesh = meshes.add_mesh(tile_vertices, tile_indices, commands);
    const auto& vertex_buffer = *meshes.get_vertex_bindings()[0].buffer;

    {
        const auto barriers = std::vector{CD3DX12_RESOURCE_BARRIER::Transition(vertex_buffer.resource.Get(),
                                                                               D3D12_RESOURCE_STATE_COPY_DEST,
                                                                               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                                          CD3DX12_RESOURCE_BARRIER::Transition(meshes.get_index_buffer().resource.Get(),
                                                                               D3D12_RESOURCE_STATE_COPY_DEST,
                                                                               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)};
        commands->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    }

    const auto ray_geo = renderer->create_raytracing_geometry(vertex_buffer, meshes.get_index_buffer(), {tile_mesh}, commands);

    device.submit_command_list(commands);

    renderer->add_raytracing_objects_to_scene({rhi::RaytracingObject{.geometry_handle = ray_geo, .material = {0}}});

    const auto tile_entity = registry->create();

    registry->assign<renderer::StandardRenderableComponent>(tile_entity, tile_mesh, terrain_material);

    loaded_terrain_tiles.emplace(tilecoord, TerrainTile{tile_heightmap, tilecoord, tile_entity});
}

std::vector<std::vector<float>> Terrain::generate_terrain_heightmap(const glm::ivec2& top_left,
                                                                    const glm::uvec2& size) const {
    const auto height_range = max_terrain_height - min_terrain_height;

    auto heightmap = std::vector<std::vector<float>>(size.y, std::vector<float>(size.x));

    auto raw_noise = std::vector<float>(size.y * size.x);

    noise_generator->FillNoiseSet(raw_noise.data(), top_left.x, top_left.y, 1, size.x, size.y, 1);

    for(uint32_t y = 0; y < size.y; y++) {
        for(uint32_t x = 0; x < size.x; x++) {
            heightmap[y][x] = raw_noise[y * size.x + x] * height_range + min_terrain_height;
        }
    }

    return heightmap;
}

glm::vec3 Terrain::get_normal_at_location(const glm::vec2& location) {
    const auto height_top_middle = get_terrain_height(location + glm::vec2{1, 0});
    const auto height_middle_left = get_terrain_height(location + glm::vec2{0, -1});
    const auto height_middle_right = get_terrain_height(location + glm::vec2{0, 1});
    const auto height_bottom_middle = get_terrain_height(location + glm::vec2{ -1, 0});
    
    const auto va = normalize(glm::vec3{2.0, 0.0, height_middle_right - height_middle_left});
    const auto vb = normalize(glm::vec3{0.0, 2.0, height_bottom_middle - height_top_middle});
    const auto normal = normalize(cross(va, vb));
    return {normal.x, normal.z, -normal.y};
}
