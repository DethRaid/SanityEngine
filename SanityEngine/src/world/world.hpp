#pragma once

#include <entt/entity/fwd.hpp>
#include <entt/entity/observer.hpp>
#include <rx/core/types.h>
#include <rx/core/vector.h>

#include "scripting/scripting_runtime.hpp"
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

class World {
public:
    /*!
     * \brief Created a world with the provided parameters
     */
    static Rx::Ptr<World> create(const WorldParameters& params,
                                 entt::entity player,
                                 entt::registry& registry,
                                 renderer::Renderer& renderer);

    void tick(Float32 delta_time);

    [[nodiscard]] Terrain& get_terrain();

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

    static Rx::Vector<Float32> generate_terrain_heightmap(FastNoiseSIMD& noise_generator, const WorldParameters& params);

    /*!
     * \brief Runs the Sanity Engine's climate model on the provided world data
     */
    static void generate_climate_data(const Rx::Vector<Float32>& heightmap, const WorldParameters& params, renderer::Renderer& renderer);

    glm::uvec2 size;

    Rx::Ptr<FastNoiseSIMD> noise_generator;

    entt::entity player;

    entt::registry* registry;

    entt::observer observer{};

    renderer::Renderer* renderer;

    Terrain terrain;

    WrenHandle* handle;

    explicit World(const glm::uvec2& size_in,
                   Uint32 min_terrain_height,
                   Uint32 max_terrain_height,
                   Rx::Ptr<FastNoiseSIMD> noise_generator_in,
                   entt::entity player_in,
                   entt::registry& registry_in,
                   renderer::Renderer& renderer_in);

    void register_component(horus::Component& component);

    /*!
     * \brief Loads the terrain tiles around the player
     *
     * First loads the tiles in the player's frustum, from closest to farthest, then loads the tiles around the player
     */
    void load_terrain_around_player();

    void tick_script_components(Float32 delta_time);
};
