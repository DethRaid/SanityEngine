#include "terrain.hpp"

#include <entt/entity/registry.hpp>

#include "generation/world_generation.hpp"
#include "../rhi/render_device.hpp"

Terrain::Terrain(renderer::Renderer& renderer_in, renderer::HostTexture2D& noise_texture_in, entt::registry& registry_in)
    : renderer{&renderer_in}, noise_texture{&noise_texture_in}, registry{&registry_in} {}

void Terrain::load_terrain_around_player(const TransformComponent& player_transform) {
    const auto coords_of_tile_containing_player = get_coords_of_tile_containing_position(player_transform.position);

    // V0: load the tile the player is in and nothing else

    // V1: load the tiles in the player's frustum, plus a few on either side so it's nice and fast for the player to spin around

    // TODO: Define some maximum number of tiles that may be loaded/generated in a given frame
    // Also TODO: Generate terrain tiles in in separate threads as part of the tasking system
    is_tile_loaded(coords_of_tile_containing_player);
}

glm::uvec2 Terrain::get_coords_of_tile_containing_position(const glm::vec3& position) {
    return glm::uvec2{static_cast<uint32_t>(position.x), static_cast<uint32_t>(position.y)} / TILE_SIZE;
}

bool Terrain::is_tile_loaded(const glm::uvec2& tilecoord) {
    if(loaded_terrain_tiles.contains(tilecoord)) {
        // Tile is already loaded, our work here is done
        return true;
    }

    generate_tile(tilecoord);

    return false;
}

void Terrain::generate_tile(const glm::uvec2& tilecoord) {
    const auto top_left = tilecoord * TILE_SIZE;
    const auto size = glm::uvec2{TILE_SIZE};

    const auto tile_heightmap = generate_terrain_heightmap(top_left, size, *noise_texture);

    std::vector<StandardVertex> tile_vertices;
    tile_vertices.reserve(tile_heightmap.size() * tile_heightmap[0].size());

    std::vector<uint32_t> tile_indices;
    tile_indices.reserve(tile_vertices.size() * 6);

    for(uint32_t y = 0; y < tile_heightmap.size(); y++) {
        const auto& tile_heightmap_row = tile_heightmap[y];
        for(uint32_t x = 0; x < tile_heightmap_row.size(); x++) {
            const auto z = tile_heightmap_row[x];

            tile_vertices.push_back(StandardVertex{.position = {x, y, z}, .normal = {0, 0, 1}, .color = 0xFF808080, .texcoord = {x, y}});

            if(x < tile_heightmap_row.size() - 1 && y < tile_heightmap.size() - 1) {
                const auto face_start_idx = x * y;

                tile_indices.push_back(face_start_idx);
                tile_indices.push_back(face_start_idx + 1);
                tile_indices.push_back(face_start_idx + tile_heightmap_row.size());

                tile_indices.push_back(face_start_idx + 1);
                tile_indices.push_back(face_start_idx + tile_heightmap_row.size());
                tile_indices.push_back(face_start_idx + tile_heightmap_row.size() + 1);
            }
        }
    }

    auto& device = renderer->get_render_device();
    const auto commands = device.create_command_list();
    const auto tile_mesh = renderer->get_static_mesh_store().add_mesh(tile_vertices, tile_indices, commands);
    device.submit_command_list(commands);

    const auto tile_entity = registry->create();

    registry->assign<renderer::StaticMeshRenderableComponent>(tile_entity, tile_mesh, terrain_material);

    loaded_terrain_tiles.emplace(tilecoord, tile_heightmap, tilecoord, tile_entity);
}
