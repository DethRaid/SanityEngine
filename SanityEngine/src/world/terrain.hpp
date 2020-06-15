#pragma once

#include <ftl/atomic_counter.h>
#include <ftl/task.h>
#include <glm/gtx/hash.hpp>
#include <rx/core/map.h>
#include <rx/core/vector.h>

#include "noise/FastNoiseSIMD/FastNoiseSIMD.h"
#include "renderer/renderer.hpp"

struct TerrainSamplerParams {
    double latitude{};
    double longitude{};
    Float32 spread{0.5};
    Float32 spread_reduction_rate{spread};
};

namespace renderer {
    class HostTexture2D;
}

struct TerrainTile {
    Rx::Vector<Rx::Vector<Float32>> heightmap;

    Vec2i coord;

    entt::entity entity;
};

class Terrain;

struct TerrainTileComponent {
    Vec2i coords;

    Terrain* terrain;
};

/*!
 * \brief Best terrain class in the entire multiverse
 *
 * The terrain has a resolution of one meter. This terrain class is made for a game that uses voxels with a resolution of one meter, so
 * everything should be groovy
 */
class Terrain {
public:
    // TODO: Make this configurable
    constexpr static int32_t TILE_SIZE = 64;

    static [[nodiscard]] Vec2i get_coords_of_tile_containing_position(const Vec3f& position);

    explicit Terrain(Uint32 max_latitude_in,
                     Uint32 max_longitude_in,
                     Uint32 min_terrain_height_in,
                     Uint32 max_terrain_height_in,
                     renderer::Renderer& renderer_in,
                     FastNoiseSIMD& noise_generator_in,
                     entt::registry& registry_in);

    void load_terrain_around_player(const TransformComponent& player_transform);

    [[nodiscard]] Float32 get_terrain_height(const Vec2f& location) const;

    [[nodiscard]] Vec3f get_normal_at_location(const Vec2f& location) const;

private:
    renderer::Renderer* renderer;

    FastNoiseSIMD* noise_generator;

    entt::registry* registry;

    Rx::Map<Vec2i, TerrainTile> loaded_terrain_tiles;

    renderer::StandardMaterialHandle terrain_material{1};

    Uint32 max_latitude;

    Uint32 max_longitude;

    Uint32 min_terrain_height;

    Uint32 max_terrain_height;

    void load_terrain_textures_and_create_material();

    void generate_tile(const Vec2i& tilecoord);

    /*!
     * \brief Generates a terrain heightmap of a specific size
     *
     * \param top_left World x and y coordinates of the top left of this terrain heightmap
     * \param size Size in world units of this terrain heightmap
     */
    [[nodiscard]] Rx::Vector<Rx::Vector<Float32>> generate_terrain_heightmap(const Vec2i& top_left, const Vec2u& size) const;
};
