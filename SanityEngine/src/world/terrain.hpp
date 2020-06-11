#pragma once

#include <ftl/atomic_counter.h>
#include <ftl/task.h>
#include <glm/gtx/hash.hpp>

#include "../renderer/renderer.hpp"

struct TerrainSamplerParams {
    double latitude{};
    double longitude{};
    float spread{0.75};
    float spread_reduction_rate{spread};
};

namespace renderer {
    class HostTexture2D;
}

struct TerrainTile {
    std::vector<std::vector<float>> heightmap;

    glm::uvec2 coord;

    entt::entity entity;
};

class Terrain;

struct TerrainTileComponent {
    glm::uvec2 coords;

    Terrain* terrain;
};

class Terrain {
public:
    // TODO: Make this configurable
    constexpr static uint32_t TILE_SIZE = 64;

    static std::shared_ptr<spdlog::logger> logger;

    static [[nodiscard]] glm::uvec2 get_coords_of_tile_containing_position(const glm::vec3& position);

    explicit Terrain(uint32_t max_latitude_in,
                     uint32_t max_longitude_in,
                     uint32_t min_terrain_height_in,
                     uint32_t max_terrain_height_in,
                     renderer::Renderer& renderer_in,
                     renderer::HostTexture2D& noise_texture_in,
                     entt::registry& registry_in);

    void load_terrain_around_player(const TransformComponent& player_transform);

    /*!
     * \brief Gets the terrain height at a specific location
     */
    [[nodiscard]] float get_terrain_height(const TerrainSamplerParams& params) const;

    [[nodiscard]] glm::vec3 get_normal_at_location(float y, float x) const;

private:
    renderer::Renderer* renderer;

    renderer::HostTexture2D* noise_texture;

    entt::registry* registry;

    std::unordered_map<glm::uvec2, TerrainTile> loaded_terrain_tiles;

    renderer::StandardMaterialHandle terrain_material{1};

    uint32_t max_latitude;

    uint32_t max_longitude;

    uint32_t min_terrain_height;

    uint32_t max_terrain_height;

    void load_terrain_textures_and_create_material();

    void generate_tile(const glm::uvec2& tilecoord);

    /*!
     * \brief Generates a terrain heightmap of a specific size
     *
     * \param top_left World x and y coordinates of the top left of this terrain heightmap
     * \param size Size in world units of this terrain heightmap
     */
    [[nodiscard]] std::vector<std::vector<float>> generate_terrain_heightmap(const glm::uvec2& top_left, const glm::uvec2& size) const;

    static FTL_TASK_ENTRY_POINT(generate_tile);
};
