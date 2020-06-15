#include "world.hpp"

#include <Tracy.hpp>
#include <ftl/task_scheduler.h>
#include <rx/core/log.h>
#include <rx/core/prng/mt19937.h>
#include <rx/math/vec2.h>

#include "core/components.hpp"
#include "core/types.hpp"
#include "generation/dual_contouring.hpp"
#include "rhi/render_device.hpp"

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

    auto terrain_data = generate_terrain(*noise_generator, params, renderer);

    generate_climate_data(terrain_data, params, renderer);

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
    ZoneScopedN("tick");

    upload_new_chunk_meshes();

    const auto& player_transform = registry->get<TransformComponent>(player);
    load_chunks_around_player(player_transform);

    tick_script_components(delta_time);
}

Terrain& World::get_terrain() { return terrain; }

// ReSharper disable once CppInconsistentNaming
WrenHandle* World::_get_wren_handle() const { return handle; }

World::TerrainData World::generate_terrain(FastNoiseSIMD& noise_generator, const WorldParameters& params, renderer::Renderer& renderer) {
    ZoneScopedN("generate_terrain_heightmap");
    auto& device = renderer.get_render_device();
    const auto commands = device.create_command_list(
        Rx::Globals::find("SanityEngine")->find("TaskScheduler")->cast<ftl::TaskScheduler>()->GetCurrentThreadIndex());

    TracyD3D12Zone(rhi::RenderDevice::tracy_context, commands.Get(), "GenerateTerrain");

    auto data = TerrainData{};

    // Generate heightmap
    const auto total_pixels_in_maps = params.width * params.height;
    data.heightmap = Rx::Vector<Float32>{total_pixels_in_maps};
    generate_heightmap(noise_generator, params, renderer, commands, data, total_pixels_in_maps);

    // Place water sources
    place_water_sources(params, renderer, commands, data, total_pixels_in_maps);

    // Let water flow around
    compute_water_flow(renderer, commands, data);

    device.submit_command_list(commands);

    return data;
}

void World::generate_heightmap(FastNoiseSIMD& noise_generator,
                               const WorldParameters& params,
                               renderer::Renderer& renderer,
                               const ComPtr<ID3D12GraphicsCommandList4> commands,
                               TerrainData& data,
                               const unsigned total_pixels_in_maps) {
    auto* height_noise = noise_generator.GetNoiseSet(-params.width / 2, -params.height / 2, 0, params.width, params.height, 1);

    memcpy(data.heightmap.data(), height_noise, total_pixels_in_maps * sizeof(Float32));

    const auto min_terrain_height = params.min_terrain_depth_under_ocean;
    const auto max_terrain_height = params.min_terrain_depth_under_ocean + params.max_ocean_depth + params.max_height_above_sea_level;
    const auto height_range = max_terrain_height - min_terrain_height;

    data.heightmap.each_fwd([&](Float32& height) { height = height * height_range + min_terrain_height; });

    data.heightmap_handle = renderer.create_image(rhi::ImageCreateInfo{.name = "Terrain Heightmap",
                                                                       .usage = rhi::ImageUsage::UnorderedAccess,
                                                                       .format = rhi::ImageFormat::R32F,
                                                                       .width = params.width,
                                                                       .height = params.height},
                                                  data.heightmap.data(),
                                                  commands);
}

void World::place_water_sources(const WorldParameters& params,
                                renderer::Renderer& renderer,
                                const ComPtr<ID3D12GraphicsCommandList4>& commands,
                                TerrainData& data,
                                const unsigned total_pixels_in_maps) {
    Rx::Vector<Float32> water_depth_map{total_pixels_in_maps};

    constexpr Float32 water_source_spawn_rate = 0.0001f;

    const auto num_water_sources = static_cast<Float32>(total_pixels_in_maps) * water_source_spawn_rate;

    Rx::Vector<glm::uvec2> water_source_locations{static_cast<Uint32>(num_water_sources)};

    Rx::PRNG::MT19937 random_number_generator;
    random_number_generator.seed(params.seed);

    water_source_locations.each_fwd([&](glm::uvec2& location) {
        const auto x = round(random_number_generator.f32() * params.width);
        const auto y = round(random_number_generator.f32() * params.height);
        location = {static_cast<Uint32>(x), static_cast<Uint32>(y)};

        water_depth_map[location.y * params.width + location.x] = 1;
    });

    data.ground_water_handle = renderer.create_image(rhi::ImageCreateInfo{.name = "Terrain Water Map",
                                                                          .usage = rhi::ImageUsage::UnorderedAccess,
                                                                          .format = rhi::ImageFormat::Rg16F,
                                                                          .width = params.width,
                                                                          .height = params.height},
                                                     water_depth_map.data(),
                                                     commands);
}

void World::compute_water_flow(renderer::Renderer& renderer, const ComPtr<ID3D12GraphicsCommandList4>& commands, TerrainData& data) {
    const auto& heightmap_image = renderer.get_image(data.heightmap_handle);
    const auto& watermap_image = renderer.get_image(data.ground_water_handle);

    {
        const Rx::Vector<D3D12_RESOURCE_BARRIER>
            barriers = Rx::Array{CD3DX12_RESOURCE_BARRIER::Transition(heightmap_image.resource.Get(),
                                                                      D3D12_RESOURCE_STATE_COPY_DEST,
                                                                      D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
                                                                          D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                                 CD3DX12_RESOURCE_BARRIER::Transition(watermap_image.resource.Get(),
                                                                      D3D12_RESOURCE_STATE_COPY_DEST,
                                                                      D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
                                                                          D3D12_RESOURCE_STATE_UNORDERED_ACCESS)};

        commands->ResourceBarrier(static_cast<Uint32>(barriers.size()), barriers.data());
    }
}

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
      terrain{size_in.y / 2, size_in.x / 2, min_terrain_height, max_terrain_height, renderer_in, *noise_generator, registry_in},
      task_scheduler{Rx::Globals::find("SanityEngine")->find("TaskScheduler")->cast<ftl::TaskScheduler>()},
      chunk_generation_fibtex{ftl::Fibtex{task_scheduler}},
      chunk_modified_event{task_scheduler} {}

void World::register_component(horus::Component& component) { component.begin_play(*this); }

Size World::get_next_free_chunk_gen_task_idx() {
    if(available_generate_chunk_blocks_task_args.is_empty()) {
        return generate_chunk_blocks_args_pool.size();
    }

    const auto index = available_generate_chunk_blocks_task_args.last();
    available_generate_chunk_blocks_task_args.pop_back();

    return index;
}

void World::ensure_chunk_at_position_is_loaded(const glm::vec3& location) {
    const auto chunk_x = static_cast<Int32>((location.x / Chunk::WIDTH) * Chunk::WIDTH);
    const auto chunk_y = static_cast<Int32>((location.z / Chunk::DEPTH) * Chunk::DEPTH);

    const auto chunk_location = Vec2i{chunk_x, chunk_y};

    Rx::Concurrency::ScopeLock l{chunk_generation_fibtex};

    if(available_chunks.find(chunk_location) != nullptr) {
        // Chunk is loaded, all is good in the world
        return;
    }

    // Chunk is not loaded, let's load it
    {
        const auto task_idx = get_next_free_chunk_gen_task_idx();
        if(task_idx == generate_chunk_blocks_args_pool.size()) {
            generate_chunk_blocks_args_pool.push_back(Rx::make_ptr<GenerateChunkBlocksArgs>(RX_SYSTEM_ALLOCATOR, chunk_location, &terrain));
        }

        auto* args = generate_chunk_blocks_args_pool[task_idx].get();
        args->task_idx_in = task_idx;
        args->location_in = chunk_location;
        args->terrain_in = &terrain;

        // Add an empty chunk. Since chunks start out marked as uninitialized, we can insert a chunk into this map to mark that the chunk
        // has started loading - the task that loads the chunk will update it's data as needed
        available_chunks.insert(chunk_location, Chunk{.location = {chunk_location.x, chunk_location.y}});

        task_scheduler->AddTask(ftl::Task{generate_blocks_for_chunk, args});
    }
}

void World::upload_new_chunk_meshes() {
    ZoneScopedN("upload_chunk_meshes");

    Rx::Concurrency::ScopeLock l{chunk_generation_fibtex};

    if(!mesh_data_ready_for_upload.is_empty()) {
        auto& device = renderer->get_render_device();
        auto commands = device.create_command_list(task_scheduler->GetCurrentThreadIndex());
        TracyD3D12Zone(rhi::RenderDevice::tracy_context, commands.Get(), "UploadChunkMeshes");

        auto& meshes = renderer->get_static_mesh_store();

        meshes.begin_adding_meshes(commands);

        mesh_data_ready_for_upload.each_pair(
            [&](const Vec2i& chunk_location, const Rx::Pair<Rx::Vector<StandardVertex>, Rx::Vector<Uint32>>& mesh_data) {
                const auto mesh_handle = meshes.add_mesh(mesh_data.first, mesh_data.second, commands);

                const auto chunk_entity = registry->create();
                registry->assign<renderer::ChunkMeshComponent>(chunk_entity, mesh_handle);

                auto& chunk = *available_chunks.find(chunk_location);
                chunk.entity = chunk_entity;
                chunk.status = Chunk::Status::MeshGenerated;
            });

        meshes.end_adding_meshes(commands);

        device.submit_command_list(commands);

        mesh_data_ready_for_upload.clear();
    }
}

void World::load_chunks_around_player(const TransformComponent& player_transform) {
    ensure_chunk_at_position_is_loaded(player_transform.location);
}

void World::dispatch_needed_mesh_gen_tasks() {
    // Check if any of the loaded chunks a) need a mesh and b) are surrounded by chunks with block data
    chunk_modified_event.CompareExchange(1, 0);

    Rx::Concurrency::ScopeLock l{chunk_generation_fibtex};
    available_chunks.each_value([&](Chunk& chunk) {
        if(chunk.status == Chunk::Status::BlockDataGenerated) {
            // This chunk needs a mesh! Do any of the chunks near this chunk need a mesh?
            bool missing_neighboring_chunks = false;

            if(const auto* neighbor_chunk = available_chunks.find({chunk.location.x - Chunk::WIDTH, chunk.location.y - Chunk::DEPTH})) {
                if(neighbor_chunk->status == Chunk::Status::Uninitialized) {
                }
            }
            missing_neighboring_chunks |= !available_chunks.find({chunk.location.x - Chunk::WIDTH, chunk.location.y});
            missing_neighboring_chunks |= !available_chunks.find({chunk.location.x - Chunk::WIDTH, chunk.location.y + Chunk::DEPTH});
            missing_neighboring_chunks |= !available_chunks.find({chunk.location.x, chunk.location.y - Chunk::DEPTH});
            missing_neighboring_chunks |= !available_chunks.find({chunk.location.x, chunk.location.y + Chunk::DEPTH});
            missing_neighboring_chunks |= !available_chunks.find({chunk.location.x + Chunk::WIDTH, chunk.location.y - Chunk::DEPTH});
            missing_neighboring_chunks |= !available_chunks.find({chunk.location.x + Chunk::WIDTH, chunk.location.y});
            missing_neighboring_chunks |= !available_chunks.find({chunk.location.x + Chunk::WIDTH, chunk.location.y + Chunk::DEPTH});

            if(missing_neighboring_chunks) {
                // At least one of the chunks in the 3x3 region centered on the current chunk doesn't have block data yet. We can't generate
                // a mesh for the current chunk just yet :(
                return RX_ITERATION_CONTINUE;
            }

            // All of the chunks in the 3x3 region centered on this chunk are fully loaded

            Size idx;
            if(available_generate_chunk_mesh_task_args.is_empty()) {
                // No available args - make a new one!
                idx = generate_chunk_mesh_args_pool.size();
                generate_chunk_mesh_args_pool.emplace_back(RX_SYSTEM_ALLOCATOR);

            } else {
                // Get index of an available args
                idx = available_generate_chunk_mesh_task_args.last();
                available_generate_chunk_mesh_task_args.pop_back();
            }

            auto* mesh_args = generate_chunk_mesh_args_pool[idx].get();

            mesh_args->task_idx_in = idx;
            mesh_args->world_in = this;
            mesh_args->location_in = chunk.location;

            task_scheduler->AddTask({generate_mesh_for_chunk_task, mesh_args});
        }
    });
}

Rx::Vector<Uint32> triangulate(const DualContouringMesh& mesh) {
    // The dual contoured mesh has quads, we want triangles
    // We triangulate the mesh by identifying the diagonal which connects the vertices with the most similar normals. Those two get
    // connected by an edge
    auto indices = Rx::Vector<Uint32>{};
    indices.reserve(mesh.faces.size() * 6);

    auto compute_normal_for_vertex = [](const Vec3f& v0, const Vec3f& v1, const Vec3f& v2) {
        const auto binormal = v1 - v0;
        const auto tangent = v2 - v0;
        return cross(binormal, tangent);
    };

    mesh.faces.each_fwd([&](const Quad& quad) {
        const auto& v1 = mesh.vertex_positions[quad.v1];
        const auto& v2 = mesh.vertex_positions[quad.v2];
        const auto& v3 = mesh.vertex_positions[quad.v3];
        const auto& v4 = mesh.vertex_positions[quad.v4];

        // Desperately hope that the mesh has a sane winding direction
        const auto n1 = compute_normal_for_vertex(v1, v4, v2);
        const auto n2 = compute_normal_for_vertex(v2, v1, v3);
        const auto n3 = compute_normal_for_vertex(v3, v2, v4);
        const auto n4 = compute_normal_for_vertex(v4, v3, v1);

        const auto odd_val = dot(n1, n3);
        const auto even_val = dot(n2, n4);

        if(odd_val < even_val) {
            // The edge between the odd vertices has less curvature
            indices.push_back(quad.v1);
            indices.push_back(quad.v2);
            indices.push_back(quad.v3);

            indices.push_back(quad.v1);
            indices.push_back(quad.v3);
            indices.push_back(quad.v4);
        } else {
            // The edge between the even vertices has less curvature
            indices.push_back(quad.v2);
            indices.push_back(quad.v3);
            indices.push_back(quad.v4);

            indices.push_back(quad.v2);
            indices.push_back(quad.v4);
            indices.push_back(quad.v1);
        }
    });

    return indices;
}

void World::generate_mesh_for_chunk(const Vec2i& chunk_location) {
    // Scan the available chunk data, extracting surface information for the current chunk
    Rx::Array<Int32[(Chunk::WIDTH + 2) * Chunk::HEIGHT * (Chunk::DEPTH + 2)]> working_data;

    // Copy block data from the relevant chunks
    {
        Rx::Concurrency::ScopeLock l{chunk_generation_fibtex};

        {
            const auto& center_chunk = *available_chunks.find(chunk_location);
            for(Uint32 z = 1; z <= Chunk::DEPTH; z++) {
                for(Uint32 y = 0; y < Chunk::HEIGHT; y++) {
                    for(Uint32 x = 1; x <= Chunk::WIDTH; x++) {
                        const auto chunk_data_idx = (x - 1) + y * Chunk::WIDTH + (z - 1) * Chunk::WIDTH * Chunk::HEIGHT;
                        const auto working_data_idx = x + y * Chunk::WIDTH + z * Chunk::WIDTH * Chunk::HEIGHT;

                        working_data[working_data_idx] = center_chunk.block_data[chunk_data_idx] == BlockRegistry::AIR ? -1 : 1;
                    }
                }
            }
        }

        {
            const auto& bottom_left_chunk = *available_chunks.find({chunk_location.x - 1, chunk_location.y - 1});
            const auto z = 0;
            const auto x = 0;
            for(Uint32 y = 0; y < Chunk::HEIGHT; y++) {
                const auto chunk_data_idx = (Chunk::WIDTH - 1) + y * Chunk::WIDTH + (Chunk::DEPTH - 1) * Chunk::WIDTH * Chunk::HEIGHT;
                const auto working_data_idx = x + y * Chunk::WIDTH + z * Chunk::WIDTH * Chunk::HEIGHT;

                working_data[working_data_idx] = bottom_left_chunk.block_data[chunk_data_idx] == BlockRegistry::AIR ? -1 : 1;
            }
        }

        {
            const auto& middle_left_chunk = *available_chunks.find({chunk_location.x - 1, chunk_location.y});
            const auto x = 0;
            for(Uint32 z = 1; z <= Chunk::DEPTH; z++) {
                for(Uint32 y = 0; y < Chunk::HEIGHT; y++) {
                    const auto chunk_data_idx = (Chunk::WIDTH - 1) + y * Chunk::WIDTH + (z - 1) * Chunk::WIDTH * Chunk::HEIGHT;
                    const auto working_data_idx = x + y * Chunk::WIDTH + z * Chunk::WIDTH * Chunk::HEIGHT;

                    working_data[working_data_idx] = middle_left_chunk.block_data[chunk_data_idx] == BlockRegistry::AIR ? -1 : 1;
                }
            }
        }

        {
            const auto& top_left_chunk = *available_chunks.find({chunk_location.x - 1, chunk_location.y + 1});
            const auto z = Chunk::DEPTH + 1;
            const auto x = 0;
            for(Uint32 y = 0; y < Chunk::HEIGHT; y++) {
                const auto chunk_data_idx = (Chunk::WIDTH - 1) + y * Chunk::WIDTH;
                const auto working_data_idx = x + y * Chunk::WIDTH + z * Chunk::WIDTH * Chunk::HEIGHT;

                working_data[working_data_idx] = top_left_chunk.block_data[chunk_data_idx] == BlockRegistry::AIR ? -1 : 1;
            }
        }

        {
            const auto& bottom_middle_chunk = *available_chunks.find({chunk_location.x, chunk_location.y - 1});
            const auto z = 0;
            for(Uint32 y = 0; y < Chunk::HEIGHT; y++) {
                for(Uint32 x = 1; x <= Chunk::WIDTH; x++) {
                    const auto chunk_data_idx = (x - 1) + y * Chunk::WIDTH + (Chunk::DEPTH - 1) * Chunk::WIDTH * Chunk::HEIGHT;
                    const auto working_data_idx = x + y * Chunk::WIDTH + z * Chunk::WIDTH * Chunk::HEIGHT;

                    working_data[working_data_idx] = bottom_middle_chunk.block_data[chunk_data_idx] == BlockRegistry::AIR ? -1 : 1;
                }
            }
        }

        {
            const auto& top_middle_chunk = *available_chunks.find({chunk_location.x, chunk_location.y + 1});
            const auto z = Chunk::DEPTH + 1;
            for(Uint32 y = 0; y < Chunk::HEIGHT; y++) {
                for(Uint32 x = 1; x <= Chunk::WIDTH; x++) {
                    const auto chunk_data_idx = (x - 1) + y * Chunk::WIDTH;
                    const auto working_data_idx = x + y * Chunk::WIDTH + z * Chunk::WIDTH * Chunk::HEIGHT;

                    working_data[working_data_idx] = top_middle_chunk.block_data[chunk_data_idx] == BlockRegistry::AIR ? -1 : 1;
                }
            }
        }
        {
            const auto& bottom_right_chunk = *available_chunks.find({chunk_location.x + 1, chunk_location.y - 1});
            const auto z = 0;
            const auto x = Chunk::WIDTH + 1;
            for(Uint32 y = 0; y < Chunk::HEIGHT; y++) {
                const auto chunk_data_idx = y * Chunk::WIDTH + (Chunk::DEPTH - 1) * Chunk::WIDTH * Chunk::HEIGHT;
                const auto working_data_idx = x + y * Chunk::WIDTH + z * Chunk::WIDTH * Chunk::HEIGHT;

                working_data[working_data_idx] = bottom_right_chunk.block_data[chunk_data_idx] == BlockRegistry::AIR ? -1 : 1;
            }
        }

        {
            const auto& middle_right_chunk = *available_chunks.find({chunk_location.x + 1, chunk_location.y});
            const auto x = Chunk::WIDTH + 1;
            for(Uint32 z = 1; z <= Chunk::DEPTH; z++) {
                for(Uint32 y = 0; y < Chunk::HEIGHT; y++) {
                    const auto chunk_data_idx = y * Chunk::WIDTH + (z - 1) * Chunk::WIDTH * Chunk::HEIGHT;
                    const auto working_data_idx = x + y * Chunk::WIDTH + z * Chunk::WIDTH * Chunk::HEIGHT;

                    working_data[working_data_idx] = middle_right_chunk.block_data[chunk_data_idx] == BlockRegistry::AIR ? -1 : 1;
                }
            }
        }

        {
            const auto& top_right_chunk = *available_chunks.find({chunk_location.x + 1, chunk_location.y + 1});
            const auto z = Chunk::DEPTH + 1;
            const auto x = Chunk::WIDTH + 1;
            for(Uint32 y = 0; y < Chunk::HEIGHT; y++) {
                const auto chunk_data_idx = y * Chunk::WIDTH;
                const auto working_data_idx = x + y * Chunk::WIDTH + z * Chunk::WIDTH * Chunk::HEIGHT;

                working_data[working_data_idx] = top_right_chunk.block_data[chunk_data_idx] == BlockRegistry::AIR ? -1 : 1;
            }
        }
    }

    // Dual-contouring
    const auto mesh = dual_contour<Chunk::WIDTH + 2, Chunk::HEIGHT, Chunk::DEPTH + 2>(working_data);

    const auto indices = triangulate(mesh);

    auto vertices = Rx::Vector<StandardVertex>{};
    vertices.reserve(mesh.vertex_positions.size());

    for(Size i = 0; i < mesh.vertex_positions.size(); i++) {
        vertices.emplace_back(mesh.vertex_positions[i], mesh.normals[i]);
    }

    Rx::Concurrency::ScopeLock l{chunk_generation_fibtex};
    mesh_data_ready_for_upload.insert(chunk_location, {Rx::Utility::move(vertices), Rx::Utility::move(indices)});
}

void World::tick_script_components(Float32 delta_time) {
    ZoneScopedN("tick_script_components");

    // registry->view<horus::Component>().each([&](horus::Component& script_component) {
    //     if(script_component.lifetime_stage == horus::LifetimeStage::ReadyToTick) {
    //         script_component.tick(delta_time);
    //     }
    // });
}

void World::generate_blocks_for_chunk(ftl::TaskScheduler* /* scheduler */, void* arg) {
    auto* args = static_cast<GenerateChunkBlocksArgs*>(arg);
    auto* world = args->world_in;

    Rx::Array<BlockId[Chunk::WIDTH * Chunk::HEIGHT * Chunk::DEPTH]> block_data;

    // Fill in block data
    // TODO: Look at the water map to place water blocks
    for(Uint32 z = 0; z < static_cast<int32_t>(Chunk::DEPTH); z++) {
        for(Uint32 x = 0; x < static_cast<int32_t>(Chunk::WIDTH); x++) {
            const auto terrain_sample_pos = Vec2f{static_cast<Float32>(args->location_in.x), static_cast<Float32>(args->location_in.y)} +
                                            Vec2f{static_cast<Float32>(x), static_cast<Float32>(z)};
            const auto height = args->terrain_in->get_terrain_height({terrain_sample_pos.x, terrain_sample_pos.y});
            const auto height_where_air_begins = glm::min(static_cast<Uint32>(round(height)), static_cast<Uint32>(Chunk::HEIGHT));

            for(Uint32 y = 0; y < height_where_air_begins; y++) {
                const auto idx = chunk_pos_to_block_index({x, y, z});
                block_data[idx] = BlockRegistry::DIRT;
            }

            for(Uint32 y = height_where_air_begins; y < Chunk::HEIGHT; y++) {
                const auto idx = chunk_pos_to_block_index({x, y, z});
                block_data[idx] = BlockRegistry::AIR;
            }
        }
    }

    {
        Rx::Concurrency::ScopeLock l{world->chunk_generation_fibtex};
        auto* chunk = world->available_chunks.find(args->location_in);
        chunk->block_data = block_data;
        chunk->status = Chunk::Status::BlockDataGenerated;
        world->chunk_modified_event.Store(1);

        world->available_generate_chunk_blocks_task_args.push_back(args->task_idx_in);
    }
}

void World::dispatch_chunk_mesh_generation_tasks(ftl::TaskScheduler* /* scheduler */, void* arg) {
    auto* args = static_cast<DispatchChunkMeshGenerationTasksArgs*>(arg);
    auto* world = args->world_in;
    // Loop infinitely, checking for chunks that need a new mesh
    while(true) {
        world->dispatch_needed_mesh_gen_tasks();
    }
}

void World::generate_mesh_for_chunk_task(ftl::TaskScheduler* /* scheduler */, void* arg) {
    // This task doesn't synchronize access to the loaded chunks. If it did, only one mesh could be built at a time. Instead, we hope and
    // pray that the block data doesn't change in the middle of trying to read it

    auto* args = static_cast<GenerateChunkMeshArgs*>(arg);
    auto* world = args->world_in;

    world->generate_mesh_for_chunk(args->location_in);

    Rx::Concurrency::ScopeLock l{world->chunk_generation_fibtex};
    world->available_generate_chunk_mesh_task_args.push_back(args->task_idx_in);
}
