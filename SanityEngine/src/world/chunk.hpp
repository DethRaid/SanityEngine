#pragma once

#include <array>
#include <cstdint>
#include <rx/core/optional.h>

#include <entt/entity/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "block_registry.hpp"

namespace renderer {
    class Renderer;
}

class Terrain;

/*!
 * \brief A single chunk in the world
 */
class Chunk {
public:
    static constexpr Uint32 WIDTH{32};
    static constexpr Uint32 HEIGHT{256};
    static constexpr Uint32 DEPTH{32};

public:
    static Rx::Optional<Chunk> create(const glm::ivec2& lower_left_corner, const Terrain& terrain, renderer::Renderer& renderer, entt::registry& registry);

    void __forceinline set_block_at_location(const glm::ivec3& location, BlockId block_id);

    [[nodiscard]] BlockId __forceinline get_block_at_location(const glm::ivec3& location) const;

private:
    static Uint32 __forceinline chunk_pos_to_block_index(const glm::uvec3& chunk_pos);

    glm::ivec2 lower_left_corner;

    /*!
     * \brief All the blocks in this chunk
     *
     * This array takes up about a MB of space, meaning you need one MB of RAM for each chunk. This is probably fine - if it ends up not
     * being fine, I'll worry about it then
     */
    std::array<BlockId, WIDTH * HEIGHT * DEPTH> block_data;

    entt::entity entity;

    explicit Chunk(const glm::ivec2& lower_left_corner_in,
                   const std::array<BlockId, WIDTH * HEIGHT * DEPTH>& block_data_in,
                   entt::entity entity_in);
};

inline void Chunk::set_block_at_location(const glm::ivec3& location, const BlockId block_id) {
    const auto x = location.x - lower_left_corner.x;
    const auto y = location.y;
    const auto z = location.z - lower_left_corner.y;

    if(x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH) {
        return;
    }

    const auto idx = chunk_pos_to_block_index({x, y, z});
    block_data[idx] = block_id;
}

inline Uint32 Chunk::chunk_pos_to_block_index(const glm::uvec3& chunk_pos) {
    return chunk_pos.x + chunk_pos.z * WIDTH + chunk_pos.y * WIDTH * DEPTH;
}
