#include "world.hpp"

#include <random>

#include <minitrace.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "../core/components.hpp"

#import "villagesim.tlb" raw_interfaces_only
#import "scriptingapi.tlb" raw_interfaces_only

std::shared_ptr<spdlog::logger> World::logger = spdlog::stdout_color_st("World");

void create_simple_boi(entt::registry& registry, ScriptingRuntime& scripting_runtime) {
    const auto entity = registry.create();

    CLSID test_component_id;
    CLSIDFromString(L"{7bde59b0-edb1-36b0-9c98-204e198e4c9f}", &test_component_id); 
    auto component = scripting_runtime.create_component(test_component_id);

    registry.assign<ScriptingComponent>(entity, component);
}

std::unique_ptr<World> World::create(const WorldParameters& params,
                                     const entt::entity player_in,
                                     entt::registry& registry_in,
                                     renderer::Renderer& renderer_in) {
    std::mt19937 rng{params.seed};

    logger->info("Creating world with seed {}", params.seed);

    auto noise_texture = renderer::HostTexture2D::create_random({256, 256}, rng);

    const auto min_terrain_height = params.min_terrain_depth_under_ocean;
    const auto max_terrain_height = params.min_terrain_depth_under_ocean + params.max_ocean_depth + params.max_height_above_sea_level;

    return std::unique_ptr<World>(new World{glm::uvec2{params.width, params.height},
                                            min_terrain_height,
                                            max_terrain_height,
                                            std::move(noise_texture),
                                            player_in,
                                            registry_in,
                                            renderer_in});
}

void World::tick(const float delta_time) {
    MTR_SCOPE("World", "tick");

    const auto& player_transform = registry->get<TransformComponent>(player);
    terrain.load_terrain_around_player(player_transform);

    tick_script_components(delta_time);
}

Terrain& World::get_terrain() { return terrain; }

World::World(const glm::uvec2& size_in,
             const uint32_t min_terrain_height,
             const uint32_t max_terrain_height,
             renderer::HostTexture2D noise_texture_in,
             entt::entity player_in,
             entt::registry& registry_in,
             renderer::Renderer& renderer_in)
    : size{size_in},
      noise_texture{std::move(noise_texture_in)},
      player{player_in},
      registry{&registry_in},
      renderer{&renderer_in},
      terrain{size_in.y / 2, size_in.x / 2, min_terrain_height, max_terrain_height, renderer_in, noise_texture, registry_in} {
}

void World::register_component(ScriptingApi::_GameplayComponentPtr component) { component->BeginPlay(/* this */); }

void World::tick_script_components(float delta_time) {
    MTR_SCOPE("World", "tick_script_components");

    registry->view<ScriptingComponent>().each([&](const ScriptingComponent& script_component) { script_component.ptr->Tick(delta_time); });
}
