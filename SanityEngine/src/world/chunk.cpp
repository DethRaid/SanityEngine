#include "chunk.hpp"

#include "terrain.hpp"

Chunk::Chunk(const glm::ivec2& lower_left_corner_in, const Terrain& terrain) : lower_left_corner{lower_left_corner_in} {
    for(int32_t z = 0; z < static_cast<int32_t>(DEPTH); z++) {
        for(int32_t x = 0; x < static_cast<int32_t>(WIDTH); x++) {
            const auto terrain_sample_pos = glm::vec2{lower_left_corner} + glm::vec2{x, z};
            const auto height = terrain.get_terrain_height(terrain_sample_pos);
            const auto height_where_air_begins = std::min(static_cast<uint32_t>(round(height)), HEIGHT);

            for(int32_t y = 0; y < height_where_air_begins; y++) {
                set_block_at_location({x, y, z}, BlockRegistry::DIRT);
            }

            for(int32_t y = height_where_air_begins; y < HEIGHT; y++) {
                set_block_at_location({x, y, z}, BlockRegistry::AIR);
            }
        }
    }
}


