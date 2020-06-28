#pragma once

#include <rx/core/concurrency/mutex.h>
#include <rx/core/map.h>
#include <rx/core/vector.h>

#include "core/async/synchronized_resource.hpp"
#include "noise/FastNoiseSIMD/FastNoiseSIMD.h"
#include "renderer/renderer.hpp"

struct WorldParameters;

struct TerrainSize {
    Uint32 max_latitude_in;
    Uint32 max_longitude_in;
    Uint32 min_terrain_height_in;
    Uint32 max_terrain_height_in;
};

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
    enum class LoadingPhase {
        GeneratingHeightmap,
        GeneratingMesh,
        Complete
    };

    LoadingPhase loading_phase{LoadingPhase::GeneratingHeightmap};

    Rx::Vector<Rx::Vector<Float32>> heightmap;

    Vec2i coord;

    entt::entity entity;
};

struct TerrainTileMeshCreateInfo {
    /*!
     * \brief Worldspace location of the tile
     */
    Vec2i coord_worldspace;

    entt::entity entity;

    Rx::Vector<StandardVertex> vertices;

    Rx::Vector<Uint32> indices;
};

class Terrain;

struct TerrainTileComponent {
    Vec2i coords;

    Terrain* terrain;
};

struct TerrainData {
    Uint32 width;
    Uint32 height;

    Rx::Vector<Float32> heightmap;

    /*!
     * \brief Handle to a texture that has the raw height values for the terrain
     */
    renderer::TextureHandle heightmap_handle;

    /*!
     * \brief Handle to a texture that stores information about water on the surface
     *
     * The depth of the water is in the red channel of this texture. This is the depth above the terrain - 0 means no water, something
     * else means some water
     *
     * We don't need to store any directions - lakes don't flow, the ocean currents are well-defined, rivers get their flow direction
     * from the slope of the terrain beneath them
     */
    renderer::TextureHandle ground_water_handle;

    /*!
     * \brief Handle to a texture that stores wind information
     *
     * The RGB of this channel is the direction of the wind, the length of that vector is the wind strength in kmph
     */
    renderer::TextureHandle wind_map_handle;

    /*!
     * \brief Handle to a texture that stores the absolute humidity on each part of the world
     */
    renderer::TextureHandle humidity_map_handle;

    /*!
     * \brief Handle to a texture that stores the soil moisture percentage
     */
    renderer::TextureHandle soil_moisture_handle;
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
    static constexpr Uint32 TILE_SIZE = 64;

    [[nodiscard]] static TerrainData generate_terrain(FastNoiseSIMD& noise_generator,
                                                      const WorldParameters& params,
                                                      renderer::Renderer& renderer);

    [[nodiscard]] static Vec2i get_coords_of_tile_containing_position(const Vec3f& position);

    explicit Terrain(const TerrainSize& size,
                     renderer::Renderer& renderer_in,
                     FastNoiseSIMD& noise_generator_in,
                     SynchronizedResource<entt::registry>& registry_in);

    void tick(float delta_time);

    void load_terrain_around_player(const TransformComponent& player_transform);

    [[nodiscard]] Float32 get_terrain_height(const Vec2f& location);

    [[nodiscard]] Vec3f get_normal_at_location(const Vec2f& location);

    [[nodiscard]] Rx::Concurrency::Atomic<Uint32>& get_num_active_tilegen_tasks();

private:
    renderer::Renderer* renderer;

    Rx::Concurrency::Mutex noise_generator_mutex;
    FastNoiseSIMD* noise_generator;

    SynchronizedResource<entt::registry>* registry;

    Rx::Concurrency::Atomic<Uint32> num_active_tilegen_tasks;

    Rx::Concurrency::Mutex loaded_terrain_tiles_mutex;
    Rx::Map<Vec2i, TerrainTile> loaded_terrain_tiles;

    SynchronizedResource<Rx::Vector<TerrainTileMeshCreateInfo>> tile_mesh_create_infos;

    renderer::StandardMaterialHandle terrain_material{1};

    Uint32 max_latitude{};

    Uint32 max_longitude{};

    Uint32 min_terrain_height;

    Uint32 max_terrain_height;

    static void generate_heightmap(FastNoiseSIMD& noise_generator,
                                   const WorldParameters& params,
                                   renderer::Renderer& renderer,
                                   const ComPtr<ID3D12GraphicsCommandList4>& commands,
                                   TerrainData& data,
                                   unsigned total_pixels_in_maps);

    static void place_water_sources(const WorldParameters& params,
                                    renderer::Renderer& renderer,
                                    const ComPtr<ID3D12GraphicsCommandList4>& commands,
                                    TerrainData& data,
                                    unsigned total_pixels_in_maps);

    static void compute_water_flow(renderer::Renderer& renderer, const ComPtr<ID3D12GraphicsCommandList4>& commands, TerrainData& data);

    void load_terrain_textures_and_create_material();

    void generate_tile(const Vec2i& tilecoord);

    /*!
     * \brief Generates a terrain heightmap of a specific size
     *
     * \param top_left World x and y coordinates of the top left of this terrain heightmap
     * \param size Size in world units of this terrain heightmap
     */
    [[nodiscard]] Rx::Vector<Rx::Vector<Float32>> generate_terrain_heightmap(const Vec2i& top_left, const Vec2u& size);

    void upload_new_tile_meshes();
};
