#pragma once

#include <entt/entity/fwd.hpp>
#include <entt/entity/observer.hpp>
#include <ftl/fibtex.h>
#include <rx/core/types.h>
#include <rx/core/vector.h>

#include "core/types.hpp"
#include "scripting/scripting_runtime.hpp"
#include "world/terrain.hpp"

namespace renderer {
    class Renderer;
}

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

    [[nodiscard]] Terrain& get_terrain() const;

#pragma region Scripting
    // ReSharper disable once CppInconsistentNaming
    [[nodiscard]] WrenHandle* _get_wren_handle() const;
#pragma endregion

private:
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

    ftl::TaskScheduler* task_scheduler;

    Rx::Ptr<Terrain> terrain;

    WrenHandle* handle;

    void register_component(horus::Component& component);

    void tick_script_components(Float32 delta_time);
};
