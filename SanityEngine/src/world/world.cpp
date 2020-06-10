#include "world.hpp"

#include <random>

#include <minitrace.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "../core/components.hpp"

std::shared_ptr<spdlog::logger> World::logger = spdlog::stdout_color_st("World");

void create_simple_boi(entt::registry& registry, horus::ScriptingRuntime& scripting_runtime) {
    const auto entity = registry.create();

    auto* handle = scripting_runtime.create_entity();

    registry.assign<WrenHandle*>(entity, handle);

    auto component = scripting_runtime.create_component(entity, "sanity_engine", "TestComponent");
    if(component) {
        registry.assign<horus::Component>(entity, *component);
    }
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

// ReSharper disable once CppInconsistentNaming
WrenHandle* World::_get_wren_handle() const { return handle; }

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
      terrain{size_in.y / 2, size_in.x / 2, min_terrain_height, max_terrain_height, renderer_in, noise_texture, registry_in} {}

void World::register_component(horus::Component& component) { component.begin_play(*this); }

void World::tick_script_components(float delta_time) {
    MTR_SCOPE("World", "tick_script_components");

    // registry->view<horus::Component>().each([&](horus::Component& script_component) {
    //     if(script_component.lifetime_stage == horus::LifetimeStage::ReadyToTick) {
    //         script_component.tick(delta_time);
    //     }
    // });
}
