#pragma once

#include <array>
#include <cstdint>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "block_registry.hpp"

class Terrain;

/*!
 * \brief A single chunk in the world
 */
class Chunk {
public:
    static constexpr uint32_t WIDTH{32};
    static constexpr uint32_t HEIGHT{256};
    static constexpr uint32_t DEPTH{32};

public:
    explicit Chunk(const glm::ivec2& lower_left_corner_in, const Terrain& terrain);

    void __forceinline set_block_at_location(const glm::ivec3& location, BlockId block_id);

    [[nodiscard]] BlockId __forceinline get_block_at_location(const glm::ivec3& location) const;

private:
    /*!
     * \brief All the blocks in this chunk
     *
     * This array takes up about a MB of space, meaning you need one MB of RAM for each chunk. This is probably fine - if it ends up not
     * being fine, I'll worry about it then
     */
    std::array<BlockId, WIDTH * HEIGHT * DEPTH> blocks;

    glm::ivec2 lower_left_corner;
};

inline void Chunk::set_block_at_location(const glm::ivec3& location, const BlockId block_id) {
    const auto x = location.x - lower_left_corner.x;
    const auto y = location.y;
    const auto z = location.z - lower_left_corner.y;

    if(x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH) {
        return;
    }

    const auto idx = x + z * WIDTH + y * WIDTH * DEPTH;
    blocks[idx] = block_id;
}
