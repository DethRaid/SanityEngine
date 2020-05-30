#include "world.hpp"

#include <random>

#include "../core/components.hpp"

std::unique_ptr<World> World::create(const WorldParameters& params,
                                     entt::entity player_in,
                                     entt::registry& registry_in,
                                     renderer::Renderer& renderer_in) {
    std::mt19937 rng{params.seed};

    auto noise_texture = renderer::HostTexture2D::create_random({256, 256}, rng);

    return std::unique_ptr<World>(
        new World{glm::uvec2{params.width, params.height}, std::move(noise_texture), player_in, registry_in, renderer_in});
}

void World::tick(float delta_time) {
    const auto& player_transform = registry->get<TransformComponent>(player);
    terrain.load_terrain_around_player(player_transform);
}

Terrain& World::get_terrain() { return terrain; }

World::World(const glm::uvec2& size_in,
             renderer::HostTexture2D noise_texture_in,
             entt::entity player_in,
             entt::registry& registry_in,
             renderer::Renderer& renderer_in)
    : size{size_in},
      noise_texture{std::move(noise_texture_in)},
      player{player_in},
      registry{&registry_in},
      renderer{&renderer_in},
      terrain{renderer_in, noise_texture, registry_in} {}
