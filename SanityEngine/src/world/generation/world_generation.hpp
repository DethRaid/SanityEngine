#pragma once
#include <vector>
#include <glm/fwd.hpp>

/*!
 * \brief A collection of things to aid in generating the world
 *
 * Some basic tasks:
 * - Generate terrain
 * - Place water sources in the terrain
 * - Simulate water flowing around, forming rivers and lakes
 * - Run a climate model vaguely based on north/south, rotation of the planet
 * - Place biomes based on climate, spawning appropriate flora and fauna
 */

constexpr float MIN_TERRAIN_HEIGHT = 32;
constexpr float MAX_TERRAIN_HEIGHT = 128;

struct TerrainSamplerParams {
    uint32_t latitude{};
    uint32_t longitude{};
    float spread{0.5};
    float spread_reduction_rate{spread};
};

namespace renderer {
    class HostTexture2D;
}

/*!
 * \brief Generates a terrain heightmap of a specific size
 *
 * \param top_left World x and y coordinates of the top left of this terrain heightmap
 * \param size Size in world units of this terrain heightmap
 * \param noise_texture Noise texture to sample for the terrain
 */
[[nodiscard]] std::vector<std::vector<float>> generate_terrain_heightmap(const glm::uvec2& top_left,
                                                                         const glm::uvec2& size,
                                                                         const renderer::HostTexture2D& noise_texture);

/*!
 * \brief Gets the terrain height at a specific location
 */
[[nodiscard]] float get_terrain_height(const TerrainSamplerParams& params, const renderer::HostTexture2D& noise_texture);


