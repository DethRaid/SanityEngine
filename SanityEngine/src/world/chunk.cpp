#include "chunk.hpp"

#include <entt/entity/registry.hpp>

#include "renderer/renderer.hpp"
#include "world/terrain.hpp"

std::optional<Chunk> Chunk::create(const glm::ivec2& lower_left_corner,
                                   const Terrain& terrain,
                                   renderer::Renderer& renderer,
                                   entt::registry& registry) {
    std::array<BlockId, WIDTH * HEIGHT * DEPTH> block_data;

    // Fill in block data
    // TODO: Get a better chunk generator which can place rivers and lakes, ore deposits, caves, villages, foliage and other biome things
    // etc
    for(int32_t z = 0; z < static_cast<int32_t>(DEPTH); z++) {
        for(int32_t x = 0; x < static_cast<int32_t>(WIDTH); x++) {
            const auto terrain_sample_pos = glm::vec2{lower_left_corner} + glm::vec2{x, z};
            const auto height = terrain.get_terrain_height(terrain_sample_pos);
            const auto height_where_air_begins = std::min(static_cast<uint32_t>(round(height)), HEIGHT);

            for(int32_t y = 0; y < height_where_air_begins; y++) {
                const auto idx = chunk_pos_to_block_index({x, y, z});
                block_data[idx] = BlockRegistry::DIRT;
            }

            for(int32_t y = height_where_air_begins; y < HEIGHT; y++) {
                const auto idx = chunk_pos_to_block_index({x, y, z});
                block_data[idx] = BlockRegistry::AIR;
            }
        }
    }

    const auto chunk_entity = registry.create();

    auto& opaque_mesh = registry.assign<renderer::ChunkMeshComponent>(chunk_entity);

    auto chunk = Chunk{lower_left_corner, block_data, chunk_entity};

    return std::optional<Chunk>{std::move(chunk)};
}

Chunk::Chunk(const glm::ivec2& lower_left_corner_in,
             const std::array<BlockId, WIDTH * HEIGHT * DEPTH>& block_data_in,
             const entt::entity entity_in)
    : lower_left_corner{lower_left_corner_in}, block_data{block_data_in}, entity{entity_in} {}
