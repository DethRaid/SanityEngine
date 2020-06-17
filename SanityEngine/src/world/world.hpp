#pragma once

#include <entt/entity/fwd.hpp>
#include <entt/entity/observer.hpp>
#include <ftl/fibtex.h>
#include <rx/core/types.h>
#include <rx/core/vector.h>

#include "core/types.hpp"
#include "scripting/scripting_runtime.hpp"
#include "world/chunk.hpp"
#include "world/terrain.hpp"

namespace renderer {
    class Renderer;
}

void create_simple_boi(entt::registry& registry, horus::ScriptingRuntime& scripting_runtime);

/*!
 * \brief Parameters for generating SanityEngine's world
 */
struct WorldParameters {
    /*!
     * \brief RNG seed to use for this world. The same seed will generate exactly the same world every time it's used
     */
    Uint32 seed;

    /*!
     * \brief Height of the world, in meters
     *
     * Height is the distance from the north end to the south end
     */
    Uint32 height;

    /*!
     * \brief Width of the world, in meters
     */
    Uint32 width;

    /*!
     * \brief Maximum depth of the ocean, in meters
     */
    Uint32 max_ocean_depth;

    /*!
     * \brief Distance from the lowest point in the ocean to the bedrock layer
     */
    Uint32 min_terrain_depth_under_ocean;

    /*
     * \brief Height above sea level of the tallest possible mountain
     *
     * If this value is negative, no land will be above the ocean and you'll be playing in a world that's 100% water. This may or may not be
     * interesting, so I'm leaving it here an an option
     */
    int32_t max_height_above_sea_level;
};

class World;

struct GenerateChunkBlocksArgs {
    World* world_in;

    Vec2i location_in;

    Terrain* terrain_in;
};

struct GenerateChunkMeshArgs {
    World* world_in;

    Vec2i location_in;
};

class World {
public:
    static constexpr Uint32 MAX_NUM_CHUNKS = 1 << 8;

    /*!
     * \brief Created a world with the provided parameters
     */
    static Rx::Ptr<World> create(const WorldParameters& params,
                                 entt::entity player,
                                 entt::registry& registry,
                                 renderer::Renderer& renderer);

    explicit World(const glm::uvec2& size_in,
                   Uint32 min_terrain_height,
                   Uint32 max_terrain_height,
                   Rx::Ptr<FastNoiseSIMD> noise_generator_in,
                   entt::entity player_in,
                   entt::registry& registry_in,
                   renderer::Renderer& renderer_in);

    void tick(Float32 delta_time);

    [[nodiscard]] Terrain& get_terrain();

    [[nodiscard]] renderer::Buffer& get_chunk_matrix_buffer() const;

#pragma region Scripting
    // ReSharper disable once CppInconsistentNaming
    [[nodiscard]] WrenHandle* _get_wren_handle() const;
#pragma endregion

private:
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

    static TerrainData generate_terrain(FastNoiseSIMD& noise_generator, const WorldParameters& params, renderer::Renderer& renderer);

    static void generate_heightmap(FastNoiseSIMD& noise_generator,
                                   const WorldParameters& params,
                                   renderer::Renderer& renderer,
                                   ComPtr<ID3D12GraphicsCommandList4> commands,
                                   TerrainData& data,
                                   unsigned total_pixels_in_maps);

    static void place_water_sources(const WorldParameters& params,
                                    renderer::Renderer& renderer,
                                    const ComPtr<ID3D12GraphicsCommandList4>& commands,
                                    TerrainData& data,
                                    unsigned total_pixels_in_maps);

    static void compute_water_flow(renderer::Renderer& renderer, const ComPtr<ID3D12GraphicsCommandList4>& commands, TerrainData& data);

    /*!
     * \brief Runs the Sanity Engine's climate model on the provided world data
     */
    static void generate_climate_data(TerrainData& heightmap, const WorldParameters& params, renderer::Renderer& renderer);

    glm::uvec2 size;

    Rx::Ptr<FastNoiseSIMD> noise_generator;

    entt::entity player;

    entt::registry* registry;

    entt::observer observer{};

    renderer::Renderer* renderer;

    Rx::Ptr<renderer::Buffer> chunk_matrix_buffer;
    Uint32 first_free_slot_in_buffer{0};

    ftl::TaskScheduler* task_scheduler;

    Terrain terrain;

    WrenHandle* handle;

    void register_component(horus::Component& component);

    void tick_script_components(Float32 delta_time);

    [[nodiscard]] bool does_chunk_have_block_data(const Vec2i& chunk_location) const;

#pragma region Chunk generation
    static void generate_blocks_for_chunk(ftl::TaskScheduler* scheduler, void* arg);

    static void generate_mesh_for_chunk_task(ftl::TaskScheduler* scheduler, void* arg);

    static void dispatch_mesh_gen_tasks(ftl::TaskScheduler* scheduler, void* arg);

    ftl::Fibtex chunk_generation_fibtex;

    /*!
     * \brief Map from chunk location to chunk for all the chunks that are fully available for use in-game
     */
    Rx::Map<Vec2i, Chunk> available_chunks;

    /*
     * \brief List of the indices of args in `args_pool` that are free to use by a new chunk generation task
     */
    Rx::Vector<Size> available_generate_chunk_blocks_task_args;

    Rx::Map<Vec2i, Rx::Pair<Rx::Vector<StandardVertex>, Rx::Vector<Uint32>>> mesh_data_ready_for_upload;

    void ensure_chunk_at_position_is_loaded(const glm::vec3& location);

    void upload_new_chunk_meshes();

    /*!
     * \brief Loads the terrain tiles around the player
     *
     * First loads the tiles in the player's frustum, from closest to farthest, then loads the tiles around the player
     */
    void load_chunks_around_player(const TransformComponent& player_transform);

    /*!
     * \brief Checks for chunks which need to have their meshes generated, and dispatches tasks for generate those meshes
     */
    void dispatch_needed_mesh_gen_tasks();

    void generate_mesh_for_chunk(const Vec2i& chunk_location);

    [[nodiscard]] renderer::ModelMatrixHandle new_chunk_matrix(const Vec2i& chunk_location);
#pragma endregion
};

/*!
 * \brief Converts from a chunk-local position to the index of the block in a chunk's block data array
 */
Uint32 __forceinline chunk_pos_to_block_index(const glm::uvec3& chunk_pos) {
    return chunk_pos.x + chunk_pos.z * Chunk::WIDTH + chunk_pos.y * Chunk::WIDTH * Chunk::DEPTH;
}
