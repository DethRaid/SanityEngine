#include "world.hpp"

#include <minitrace.h>
#include <rx/core/log.h>

#include "core/components.hpp"

RX_LOG("World", logger);

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
    logger->info("Creating world with seed %d", params.seed);

    auto noise_generator = Rx::Ptr<FastNoiseSIMD>{Rx::Memory::SystemAllocator::instance(), FastNoiseSIMD::NewFastNoiseSIMD(params.seed)};

    // Settings gotten from messing around in the demo application. High chance these should be tuned in-game
    noise_generator->SetNoiseType(FastNoiseSIMD::PerlinFractal);
    noise_generator->SetFrequency(1.0f / 64.0f);
    noise_generator->SetFractalType(FastNoiseSIMD::FBM);
    noise_generator->SetFractalOctaves(10);
    noise_generator->SetFractalLacunarity(2);
    noise_generator->SetFractalGain(0.5f);

    const auto min_terrain_height = params.min_terrain_depth_under_ocean;
    const auto max_terrain_height = params.min_terrain_depth_under_ocean + params.max_ocean_depth + params.max_height_above_sea_level;

    const auto heightmap = generate_terrain_heightmap(*noise_generator, params);

    generate_climate_data(heightmap, params, renderer);

    return Rx::make_ptr<World>(Rx::Memory::SystemAllocator::instance(),
                               glm::uvec2{params.width, params.height},
                               min_terrain_height,
                               max_terrain_height,
                               std::move(noise_generator),
                               player,
                               registry,
                               renderer);
}

void World::tick(const Float32 delta_time) {
    MTR_SCOPE("World", "tick");

    const auto& player_transform = registry->get<TransformComponent>(player);
    terrain.load_terrain_around_player(player_transform);

    tick_script_components(delta_time);
}

Terrain& World::get_terrain() { return terrain; }

// ReSharper disable once CppInconsistentNaming
WrenHandle* World::_get_wren_handle() const { return handle; }

Rx::Vector<Float32> World::generate_terrain_heightmap(FastNoiseSIMD& noise_generator, const WorldParameters& params) {
    const auto num_height_samples = params.width * 2 * params.height * 2;
    auto* height_noise = noise_generator.GetNoiseSet(-params.width, -params.height, 0, params.width * 2, params.height * 2, 1);

    auto return_value = Rx::Vector<Float32>{num_height_samples};
    memcpy(return_value.data(), height_noise, num_height_samples * sizeof(Float32));

    const auto min_terrain_height = params.min_terrain_depth_under_ocean;
    const auto max_terrain_height = params.min_terrain_depth_under_ocean + params.max_ocean_depth + params.max_height_above_sea_level;
    const auto height_range = max_terrain_height - min_terrain_height;

    return_value.each_fwd([&](Float32& height) { height = height * height_range + min_terrain_height; });

    // Todo: 

    return return_value;
}

void World::generate_climate_data(const Rx::Vector<Float32>& heightmap, const WorldParameters& params, renderer::Renderer& renderer) {
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
      terrain{size_in.y / 2, size_in.x / 2, min_terrain_height, max_terrain_height, renderer_in, *noise_generator, registry_in} {}

void World::register_component(horus::Component& component) { component.begin_play(*this); }

void World::tick_script_components(Float32 delta_time) {
    MTR_SCOPE("World", "tick_script_components");

    // registry->view<horus::Component>().each([&](horus::Component& script_component) {
    //     if(script_component.lifetime_stage == horus::LifetimeStage::ReadyToTick) {
    //         script_component.tick(delta_time);
    //     }
    // });
}
