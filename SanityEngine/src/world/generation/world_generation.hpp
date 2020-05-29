#pragma once

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

/*!
 * \brief Number of blocks from the equator to the north pole
 *
 * Half the number of blocks from the north pole to the south pole
 */
constexpr float TERRAIN_LATITUDE_RANGE = 32768;

/*!
 * \brief Half the number of blocks from the westernmost edge of the world to the easternmost edge of the world
 */
constexpr float TERRAIN_LONGITUDE_RANGE = TERRAIN_LATITUDE_RANGE * 2;

struct TerrainSamplerParams {
    float latitude;
    float longitude;
    float spread{0.5};
    float spread_reduction_rate{spread};
};

namespace renderer {
    class Texture2D;
}

/*!
 * \brief Gets the terrain height
 */
[[nodiscard]] float get_terrain_height(const TerrainSamplerParams& params, const renderer::Texture2D& noise_texture);


