#pragma once

#include <glm/gtx/hash.hpp>

#include "../renderer/renderer.hpp"

struct TerrainSamplerParams;

namespace renderer {
    class HostTexture2D;
}

struct TerrainTile {
    std::vector<std::vector<float>> heightmap;

    glm::uvec2 coord;

    entt::entity entity;
};

class Terrain {
public:
    // TODO: Make this configurable
    constexpr static uint32_t TILE_SIZE = 64;

    static [[nodiscard]] glm::uvec2 get_coords_of_tile_containing_position(const glm::vec3& position);

    explicit Terrain(uint32_t max_latitude_in,
                     uint32_t max_longitude_in,
                     uint32_t min_terrain_height_in,
                     uint32_t max_terrain_height_in,
                     renderer::Renderer& renderer_in,
                     renderer::HostTexture2D& noise_texture_in,
                     entt::registry& registry_in);

    void load_terrain_around_player(const TransformComponent& player_transform);

private:
    static std::shared_ptr<spdlog::logger> logger;

    renderer::Renderer* renderer;

    renderer::HostTexture2D* noise_texture;

    entt::registry* registry;

    std::unordered_map<glm::uvec2, TerrainTile> loaded_terrain_tiles;

    renderer::MaterialHandle terrain_material{1};

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

    /*!
     * \brief Gets the terrain height at a specific location
     */
    [[nodiscard]] float get_terrain_height(const TerrainSamplerParams& params) const;
};
