#pragma once

#include <entt/entity/fwd.hpp>
#include <entt/entity/observer.hpp>

#include "renderer/textures.hpp"
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
    uint32_t seed;

    /*!
     * \brief Height of the world, in meters
     *
     * Height is the distance from the north end to the south end
     */
    uint32_t height;

    /*!
     * \brief Width of the world, in meters
     */
    uint32_t width;

    /*!
     * \brief Maximum depth of the ocean, in meters
     */
    uint32_t max_ocean_depth;

    /*!
     * \brief Distance from the lowest point in the ocean to the bedrock layer
     */
    uint32_t min_terrain_depth_under_ocean;

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
    static std::unique_ptr<World> create(const WorldParameters& params,
                                         entt::entity player_in,
                                         entt::registry& registry_in,
                                         renderer::Renderer& renderer_in);

    void tick(float delta_time);

    [[nodiscard]] Terrain& get_terrain();

#pragma region Scripting
    // ReSharper disable once CppInconsistentNaming
    [[nodiscard]] WrenHandle* _get_wren_handle() const;
#pragma endregion

private:
    static std::shared_ptr<spdlog::logger> logger;

    glm::uvec2 size;

    std::unique_ptr<FastNoiseSIMD> noise_generator;

    entt::entity player;

    entt::registry* registry;

    entt::observer observer{};

    renderer::Renderer* renderer;

    Terrain terrain;

    WrenHandle* handle;

    explicit World(const glm::uvec2& size_in,
                   uint32_t min_terrain_height,
                   uint32_t max_terrain_height,
                   std::unique_ptr<FastNoiseSIMD> noise_generator_in,
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

    void tick_script_components(float delta_time);
};
