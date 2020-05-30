#pragma once

#include "../renderer/renderer.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

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
    constexpr static uint32_t TILE_SIZE = 512;

    static [[nodiscard]] glm::uvec2 get_coords_of_tile_containing_position(const glm::vec3& position);
    
    explicit Terrain(renderer::Renderer& renderer_in, renderer::HostTexture2D& noise_texture_in, entt::registry& registry_in);

    void load_terrain_around_player(const TransformComponent& player_transform);

    bool is_tile_loaded(const glm::uvec2& tilecoord);

private:
    renderer::Renderer* renderer;

    renderer::HostTexture2D* noise_texture;

    entt::registry* registry;

    std::unordered_map<glm::uvec2, TerrainTile> loaded_terrain_tiles;

    renderer::MaterialHandle terrain_material{0};

    void generate_tile(const glm::uvec2& tilecoord);
};
