#include "world.hpp"

#include <Tracy.hpp>
#include <ftl/atomic_counter.h>
#include <ftl/task_scheduler.h>
#include <rx/core/log.h>
#include <rx/core/prng/mt19937.h>
#include <rx/math/vec2.h>

#include "core/components.hpp"
#include "core/types.hpp"
#include "generation/dual_contouring.hpp"
#include "rhi/render_device.hpp"

RX_LOG("World", logger);
RX_LOG("ChunkMeshGenTaskDispatcher", logger_dispatch);

void create_simple_boi(entt::registry& registry, horus::ScriptingRuntime& scripting_runtime) {
    const auto entity = registry.create();

    auto* handle = scripting_runtime.create_entity();

    registry.assign<WrenHandle*>(entity, handle);

    auto component = scripting_runtime.create_component(entity, "sanity_engine", "TestComponent");
    if(component) {
        registry.assign<horus::Component>(entity, *component);
    }
}

Rx::Ptr<World> World::create(const WorldParameters& params,
                             const entt::entity player,
                             entt::registry& registry,
                             renderer::Renderer& renderer) {
    ZoneScoped;

    logger->info("Creating world with seed %d", params.seed);

    auto noise_generator = Rx::Ptr<FastNoiseSIMD>{RX_SYSTEM_ALLOCATOR, FastNoiseSIMD::NewFastNoiseSIMD(params.seed)};

    // Settings gotten from messing around in the demo application. High chance these should be tuned in-game
    noise_generator->SetNoiseType(FastNoiseSIMD::PerlinFractal);
    noise_generator->SetFrequency(1.0f / 64.0f);
    noise_generator->SetFractalType(FastNoiseSIMD::FBM);
    noise_generator->SetFractalOctaves(10);
    noise_generator->SetFractalLacunarity(2);
    noise_generator->SetFractalGain(0.5f);

    const auto min_terrain_height = params.min_terrain_depth_under_ocean;
    const auto max_terrain_height = params.min_terrain_depth_under_ocean + params.max_ocean_depth + params.max_height_above_sea_level;

    auto terrain_data = Terrain::generate_terrain(*noise_generator, params, renderer);

    generate_climate_data(terrain_data, params, renderer);

    return Rx::make_ptr<World>(RX_SYSTEM_ALLOCATOR,
                               glm::uvec2{params.width, params.height},
                               min_terrain_height,
                               max_terrain_height,
                               std::move(noise_generator),
                               player,
                               registry,
                               renderer);
}

World::World(const glm::uvec2& size_in,
             const Uint32 min_terrain_height,
             const Uint32 max_terrain_height,
             Rx::Ptr<FastNoiseSIMD> noise_generator_in,
             entt::entity player_in,
             entt::registry& registry_in,
             renderer::Renderer& renderer_in)
    : size{size_in},
      noise_generator{std::move(noise_generator_in)},
      player{player_in},
      registry{&registry_in},
      renderer{&renderer_in},
      task_scheduler{Rx::Globals::find("SanityEngine")->find("TaskScheduler")->cast<ftl::TaskScheduler>()},
      terrain{Rx::make_ptr<Terrain>(RX_SYSTEM_ALLOCATOR,
                                    TerrainSize{size_in.y / 2, size_in.x / 2, min_terrain_height, max_terrain_height},
                                    renderer_in,
                                    *noise_generator,
                                    registry_in,
                                    *task_scheduler)} {}

void World::tick(const Float32 delta_time) {
    ZoneScoped;

    const auto& player_transform = registry->get<TransformComponent>(player);
    terrain->load_terrain_around_player(player_transform);

    tick_script_components(delta_time);
}

Terrain& World::get_terrain() const { return *terrain; }

// ReSharper disable once CppInconsistentNaming
WrenHandle* World::_get_wren_handle() const { return handle; }

void World::generate_climate_data(TerrainData& terrain_data, const WorldParameters& params, renderer::Renderer& renderer) {
    /*
     * Runs a basic climate simulation on the the heightmap
     *
     * We upload the heightmap to the GPU to run the climate sim in a compute shader
     *
     * Climate sim outputs are humidity, wind direction and strength, temperature range
     *
     * Wind is based on atmospheric circulation. The wind currents in Sanity Engine are very similar to the wind currents on Earth. After
     * computing the atmospheric circulation, the compute pass looks for features in the heightmap that would block the wind - mountains,
     * mostly. We search along the wind direction, looking for the horizon of the terrain. The closer and higher that horizon is, the more
     * wind it'll block. Wind gets output into a RGBA32F texture. The RGB holds the wind direction and strength - length of the vector is
     * the wind's speed in kmph
     *
     * Once we've determined wind speed and direction, we calculate humidity. There's a lot of humidity directly over water, then we give it
     * is 10km (or so) blur, then smear humidity along wind direction. Mountains will be able to block humidity in some way - probably
     * something about decreasing humidity amount by height and slope of the terrain is passes over? Humidity gets stored in a R8 texture
     *
     * Temperature range is based on latitude and wind direction. Latitude gives base temperature range - dot product multiplied by
     * temperature range at the equator - then we take wind and humidity into account. Humid winds blowing off the ocean lower the upper
     * part of the temperature range, while humidity that isn't from a body of water will both raise and lower max temperature. Temperature
     * range gets saved to a RG8 texture - min in the red, max in the green
     *
     * After that process is finished, Sanity Engine runs more compute shaders to calculate other things -
     *
     * After the computations are done, we copy those textures back to the CPU so that they can be used for biome generation
     */
}

void World::register_component(horus::Component& component) {
    ZoneScoped;

    component.begin_play(*this);
}

void World::tick_script_components(Float32 delta_time) {
    ZoneScoped;

    // registry->view<horus::Component>().each([&](horus::Component& script_component) {
    //     if(script_component.lifetime_stage == horus::LifetimeStage::ReadyToTick) {
    //         script_component.tick(delta_time);
    //     }
    // });
}
