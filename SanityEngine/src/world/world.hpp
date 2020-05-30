#pragma once

#include <entt/entity/fwd.hpp>
#include <entt/entity/observer.hpp>


#include "terrain.hpp"
#include "../renderer/textures.hpp"

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

private:
    glm::uvec2 size;

    renderer::HostTexture2D noise_texture;

    entt::entity player;

    entt::registry* registry;

    entt::observer observer{};

    renderer::Renderer* renderer;

    Terrain terrain;

    explicit World(const glm::uvec2& size_in,
                   renderer::HostTexture2D noise_texture_in,
                   entt::entity player_in,
                   entt::registry& registry_in,
                   renderer::Renderer& renderer_in);

    /*!
     * \brief Loads the terrain tiles around the player
     *
     * First loads the tiles in the player's frustum, from closest to farthest, then loads the tiles around the player
     */
    void load_terrain_around_player();
};
