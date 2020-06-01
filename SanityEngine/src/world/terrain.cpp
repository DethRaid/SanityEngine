#include "terrain.hpp"

#include <entt/entity/registry.hpp>
#include <minitrace.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "../core/ensure.hpp"
#include "../loading/image_loading.hpp"
#include "../renderer/standard_material.hpp"
#include "../renderer/textures.hpp"
#include "../rhi/render_device.hpp"

struct TerrainSamplerParams {
    uint32_t latitude{};
    uint32_t longitude{};
    float spread{0.5};
    float spread_reduction_rate{spread};
};

std::shared_ptr<spdlog::logger> Terrain::logger = spdlog::stdout_color_st("Terrain");

Terrain::Terrain(const uint32_t max_latitude_in,
                 const uint32_t max_longitude_in,
                 const uint32_t min_terrain_height_in,
                 const uint32_t max_terrain_height_in,
                 renderer::Renderer& renderer_in,
                 renderer::HostTexture2D& noise_texture_in,
                 entt::registry& registry_in)
    : renderer{&renderer_in},
      noise_texture{&noise_texture_in},
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

glm::uvec2 Terrain::get_coords_of_tile_containing_position(const glm::vec3& position) {
    return glm::uvec2{static_cast<uint32_t>(position.x), static_cast<uint32_t>(position.y)} / TILE_SIZE;
}

void Terrain::load_terrain_textures_and_create_material() {
    auto& device = renderer->get_render_device();
    const auto commands = device.create_command_list();

    renderer::TextureHandle albedo_handle{};
    renderer::TextureHandle normals_roughness_handle{};

    bool success;

    {
        MTR_SCOPE("Terrain", "Load grass albedo");
        constexpr auto texture_name = "data/textures/terrain/Ground_Forest_sfjmafua_8K_surface_ms/sfjmafua_512_Albedo.jpg";

        uint32_t width, height;
        std::vector<uint8_t> pixels;

        success = load_image(texture_name, width, height, pixels);
        if(!success) {
            logger->error("Could not load grass albedo texture {}", texture_name);
        } else {
            const auto create_info = rhi::ImageCreateInfo{.name = texture_name,
                                                          .usage = rhi::ImageUsage::SampledImage,
                                                          .format = rhi::ImageFormat::Rgba8,
                                                          .width = width,
                                                          .height = height};

            albedo_handle = renderer->create_image(create_info, pixels.data(), commands);
        }
    }

    {
        MTR_SCOPE("Terrain", "Load grass normal/roughness");
        constexpr auto
            normal_roughness_texture_name = "data/textures/terrain/Ground_Forest_sfjmafua_8K_surface_ms/sfjmafua_512_Normal_Roughness.jpg";

        uint32_t normal_roughness_width, normal_roughness_height;
        std::vector<uint8_t> normal_roughness_pixels;

        success = load_image(normal_roughness_texture_name, normal_roughness_width, normal_roughness_height, normal_roughness_pixels);
        if(!success) {
            logger->error("Could not load grass normal/roughness texture {}", normal_roughness_texture_name);

        } else {

            const auto create_info = rhi::ImageCreateInfo{.name = "Terrain normal roughness texture",
                                                          .usage = rhi::ImageUsage::SampledImage,
                                                          .format = rhi::ImageFormat::Rgba8,
                                                          .width = normal_roughness_width,
                                                          .height = normal_roughness_height};

            normals_roughness_handle = renderer->create_image(create_info, normal_roughness_pixels.data(), commands);
        }
    }

    device.submit_command_list(commands);

    if(!success) {
        logger->error("Could not load terrain textures");
        albedo_handle = renderer->get_pink_texture();
    }

    auto& materials = renderer->get_material_data_buffer();
    terrain_material = materials.get_next_free_material<StandardMaterial>();
    auto& material = materials.at<StandardMaterial>(terrain_material);
    material.albedo = albedo_handle;
    material.normal_roughness = normals_roughness_handle;
    material.noise = renderer->get_noise_texture();
}

void Terrain::generate_tile(const glm::uvec2& tilecoord) {
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

            tile_vertices.push_back(StandardVertex{.position = {x, height, y}, .normal = {0, 1, 0}, .color = 0xFF808080, .texcoord = {x, y}});

            if(x < tile_heightmap_row.size() - 1 && y < tile_heightmap.size() - 1) {
                const auto width = tile_heightmap_row.size();
                const auto face_start_idx = y * width + x;

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
    const auto tile_mesh = renderer->get_static_mesh_store().add_mesh(tile_vertices, tile_indices, commands);
    device.submit_command_list(commands);

    const auto tile_entity = registry->create();

    registry->assign<renderer::StaticMeshRenderableComponent>(tile_entity, tile_mesh, terrain_material);

    loaded_terrain_tiles.emplace(tilecoord, TerrainTile{tile_heightmap, tilecoord, tile_entity});
}

std::vector<std::vector<float>> Terrain::generate_terrain_heightmap(const glm::uvec2& top_left, const glm::uvec2& size) const {
    auto heightmap = std::vector<std::vector<float>>(size.y, std::vector<float>(size.x));

    for(uint32_t y = 0; y < size.y; y++) {
        for(uint32_t x = 0; x < size.x; x++) {
            const auto params = TerrainSamplerParams{.latitude = y + top_left.y, .longitude = x + top_left.x};
            heightmap[y][x] = get_terrain_height(params);
        }
    }

    return heightmap;
}

constexpr uint32_t NUM_OCTAVES = 5;

float Terrain::get_terrain_height(const TerrainSamplerParams& params) const {
    const static D3D12_SAMPLER_DESC NOISE_SAMPLER{.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
                                                  .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                                  .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP};
    // Generate terrain whooooooo

    // General idea:
    //
    // Sample the noise texture for a lot of octaves of noise
    // Octave 0 has a resolution of eight texels from the north pole to the south pole, and sixteen texels from the westernmost edge of the
    // world to the easternmost edge
    //
    // Octave 1 has twice the resolution of octave 0
    //
    // Octave 2 has twice the resolution of octave 1
    //
    // etc

    const glm::vec2 octave_0_scale = glm::vec2{1.0f} / glm::vec2{noise_texture->get_size() / 4u};
    glm::vec2 texcoord = glm::vec2{static_cast<float>(params.longitude) / (max_longitude * 2.0f),
                                   static_cast<float>(params.latitude) / (max_latitude * 2.0f)};
    texcoord *= octave_0_scale;

    double terrain_height = 0;
    float spread = params.spread;

    for(uint32_t i = 0; i < NUM_OCTAVES; i++) {
        const auto& noise_sample = noise_texture->sample(NOISE_SAMPLER, texcoord);
        const auto height_sample = static_cast<double>(noise_sample.x) / 255.0;
        terrain_height += height_sample * spread;

        spread *= params.spread_reduction_rate;
        texcoord *= 2;
    }

    const auto terrain_height_range = max_terrain_height - min_terrain_height;
    return static_cast<float>(terrain_height * terrain_height_range + min_terrain_height);
}
