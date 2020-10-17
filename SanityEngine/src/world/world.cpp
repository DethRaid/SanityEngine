#include "world.hpp"

#include "Tracy.hpp"
#include "adapters/rex/rex_wrapper.hpp"
#include "core/components.hpp"
#include "core/types.hpp"
#include "loading/mesh_loading.hpp"
#include "renderer/rhi/render_device.hpp"
#include "rx/core/filesystem/directory.h"
#include "rx/core/log.h"
#include "rx/core/prng/mt19937.h"
#include "rx/math/vec2.h"
#include "sanity_engine.hpp"

RX_LOG("World", logger);
RX_LOG("ChunkMeshGenTaskDispatcher", logger_dispatch);

Rx::Ptr<World> World::create(const WorldParameters& params,
                             const entt::entity player,
                             SynchronizedResource<entt::registry>& registry,
                             renderer::Renderer& renderer) {
    ZoneScoped;

    logger->info("Creating world with seed %d", params.seed);

    auto noise_generator = FastNoiseSIMD::NewFastNoiseSIMD(params.seed);

    // Settings gotten from messing around in the demo application. High chance these should be tuned in-game
    noise_generator->SetNoiseType(FastNoiseSIMD::PerlinFractal);
    noise_generator->SetFrequency(1.0f / 64.0f);
    noise_generator->SetFractalType(FastNoiseSIMD::FBM);
    noise_generator->SetFractalOctaves(10);
    noise_generator->SetFractalLacunarity(2);
    noise_generator->SetFractalGain(0.5f);

    auto terrain_data = Terrain::generate_terrain(*noise_generator, params, renderer);

    const auto min_terrain_height = params.min_terrain_depth_under_ocean;
    const auto max_terrain_height = params.min_terrain_depth_under_ocean + params.max_ocean_depth + params.max_height_above_sea_level;
    terrain_data.size = TerrainSize{params.height / 2, params.width / 2, min_terrain_height, max_terrain_height};

    generate_climate_data(terrain_data, params, renderer);

    auto terrain = Rx::make_ptr<Terrain>(RX_SYSTEM_ALLOCATOR, terrain_data, renderer, *noise_generator, registry);

    return Rx::make_ptr<World>(RX_SYSTEM_ALLOCATOR,
                               glm::uvec2{params.width, params.height},
                               noise_generator,
                               player,
                               registry,
                               renderer,
                               Rx::Utility::move(terrain));
}

World::World(const glm::uvec2& size_in,
             FastNoiseSIMD* noise_generator_in,
             const entt::entity player_in,
             SynchronizedResource<entt::registry>& registry_in,
             renderer::Renderer& renderer_in,
             Rx::Ptr<Terrain> terrain_in)
    : size{size_in},
      noise_generator{noise_generator_in},
      player{player_in},
      registry{&registry_in},
      renderer{&renderer_in},
      terrain{Rx::Utility::move(terrain_in)} {}

World::~World() { delete noise_generator; }

void World::tick(const Float64 delta_time) {
    ZoneScoped;

    {
        auto locked_registry = registry->lock();
        const auto& player_transform = locked_registry->get<TransformComponent>(player);
        terrain->load_terrain_around_player(player_transform);
    }

    terrain->tick(delta_time);
}

Terrain& World::get_terrain() const { return *terrain; }

void World::generate_climate_data(TerrainData& terrain_data, const WorldParameters& params, renderer::Renderer& renderer) {
    ZoneScoped;

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

void World::load_environment_objects(const Rx::String& environment_objects_folder) {
    const auto environment_objects_absolute_directory = Rx::String::format("%s/%s",
                                                                           SanityEngine::executable_directory,
                                                                           environment_objects_folder);

    auto& device = renderer->get_render_device();
    auto commands = device.create_command_list();

    bool loaded_anything = false;

    auto environment_objects_directory = Rx::Filesystem::Directory{environment_objects_absolute_directory};
    if(environment_objects_directory) {
        // Only try to register assets in this directory is the directory exists
        environment_objects_directory.each([&](const Rx::Filesystem::Directory::Item& item) {
            auto sub_folder = item.as_directory();
            if(sub_folder) {
                // load
            } else {
                // TODO: Define an asset format, and handle it reasonably

                const auto filepath = Rx::String::format("%s/%s", environment_objects_absolute_directory.data(), item.name());

                // For not, just yeet FBXs into memory
                const auto filename = item.name();
                if(filename.ends_with(".fbx")) {
                    const auto imported_mesh = import_mesh(filepath, commands, *renderer);
                    if(imported_mesh) {
                        loaded_anything = true;
                    }
                }
            }
        });
    }

    if(loaded_anything) {
        device.submit_command_list(Rx::Utility::move(commands));
    }
}
