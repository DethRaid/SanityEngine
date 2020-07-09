#include "terrain.hpp"

#include <Tracy.hpp>
#include <TracyD3D12.hpp>
#include <entt/entity/registry.hpp>
#include <pix3.h>
#include <rx/console/variable.h>
#include <rx/core/array.h>
#include <rx/core/log.h>
#include <rx/core/prng/mt19937.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.Threading.h>

#include "generation/gpu_terrain_generation.hpp"
#include "loading/image_loading.hpp"
#include "renderer/renderer.hpp"
#include "renderer/standard_material.hpp"
#include "rhi/helpers.hpp"
#include "rhi/render_device.hpp"
#include "sanity_engine.hpp"

using namespace winrt;
using winrt::Windows::Foundation::AsyncStatus;
using winrt::Windows::Foundation::IAsyncAction;
using winrt::Windows::System::Threading::ThreadPool;

RX_LOG("\033[32mTerrain\033[0m", logger);

RX_CONSOLE_IVAR(
    cvar_max_terrain_tile_distance, "t.MaxTileDistance", "Maximum distance at which Sanity Engine will load terrain tiles", 1, INT_MAX, 16);

RX_CONSOLE_IVAR(cvar_max_generating_terrain_tiles,
                "t.MaxGeneratingTiles",
                "Maximum number of tiles that may be concurrently generated",
                1,
                INT_MAX,
                128);

struct GenerateTileTaskArgs {
    Terrain* terrain{nullptr};

    Vec2i tilecoord{};
};

TerrainData Terrain::generate_terrain(FastNoiseSIMD& noise_generator, const WorldParameters& params, renderer::Renderer& renderer) {
    ZoneScoped;

    auto& device = renderer.get_render_device();
    auto commands = device.create_command_list();
    commands->SetName(L"Terrain::generate_terrain");

    const auto total_pixels_in_maps = params.width * params.height;
    auto data = TerrainData{.size = {.max_latitude_in = params.height / 2.0, .max_longitude_in = params.width / 2.0},
                            .heightmap = Rx::Vector<Float32>{total_pixels_in_maps}};

    {
        TracyD3D12Zone(renderer::RenderDevice::tracy_context, commands.get(), "Terrain::generate_terrain");
        PIXScopedEvent(commands.get(), PIX_COLOR_DEFAULT, "Terrain::generate_terrain");

        // Generate heightmap
        generate_heightmap(noise_generator, params, renderer, commands, data, total_pixels_in_maps);
        const auto heightmap_image = renderer.get_image(data.heightmap_handle);

        const auto heightmap_barrier = CD3DX12_RESOURCE_BARRIER::UAV(heightmap_image.resource.get());

        commands->ResourceBarrier(1, &heightmap_barrier);

        // Place water sources
        place_water_sources(params, renderer, commands, data, total_pixels_in_maps);
        const auto water_depth_image = renderer.get_image(data.water_depth_handle);

        terraingen::place_oceans(commands, renderer, params.min_terrain_depth_under_ocean + params.max_ocean_depth, data);

        // Let water flow around
        terraingen::compute_water_flow(commands, renderer, data);
    }

    device.submit_command_list(Rx::Utility::move(commands));

    return data;
}

Terrain::Terrain(const TerrainData& data,
                 renderer::Renderer& renderer_in,
                 FastNoiseSIMD& noise_generator_in,
                 SynchronizedResource<entt::registry>& registry_in)
    : renderer{&renderer_in},
      noise_generator{&noise_generator_in},
      registry{&registry_in},
      max_latitude{data.size.max_latitude_in},
      max_longitude{data.size.max_longitude_in},
      min_terrain_height{data.size.min_terrain_height_in},
      max_terrain_height{data.size.max_terrain_height_in} {

    // TODO: Make a good data structure to load the terrain material(s) at runtime
    load_terrain_textures_and_create_material();
}

void Terrain::tick(float delta_time) {
    ZoneScoped;

    upload_new_tile_meshes();
}

void Terrain::load_terrain_around_player(const TransformComponent& player_transform) {
    ZoneScoped;
    const auto coords_of_tile_containing_player = get_coords_of_tile_containing_position(
        {player_transform.location.x, player_transform.location.y, player_transform.location.z});

    // V0: load the tile the player is in and nothing else

    // V1: load the tiles in the player's frustum, plus a few on either side so it's nice and fast for the player to spin around

    // TODO: Define some maximum number of tiles that may be loaded/generated in a given frame

    Rx::Concurrency::ScopeLock l{loaded_terrain_tiles_mutex};

    if(loaded_terrain_tiles.find(coords_of_tile_containing_player) == nullptr) {
        logger->verbose("Marking tile (%d, %d) as having started loading",
                        coords_of_tile_containing_player.x,
                        coords_of_tile_containing_player.y);
        loaded_terrain_tiles.insert(coords_of_tile_containing_player, {});
        num_active_tilegen_tasks.fetch_add(1);
        ThreadPool::RunAsync([=](const IAsyncAction& /* work_item */) { generate_tile(coords_of_tile_containing_player); });
    }

    // const auto max_tile_distance = cvar_max_terrain_tile_distance->get();
    // for(Int32 distance_from_player = 1; distance_from_player < max_tile_distance; distance_from_player++) {
    //     for(Int32 chunk_y = -distance_from_player; chunk_y <= distance_from_player; chunk_y++) {
    //         for(Int32 chunk_x = -distance_from_player; chunk_x <= distance_from_player; chunk_x++) {
    //             // Only generate chunks at the edge of our current square
    //             if((chunk_y != -distance_from_player) && (chunk_y != distance_from_player) && (chunk_x != -distance_from_player) &&
    //                (chunk_x != distance_from_player)) {
    //                 continue;
    //             }
    //
    //             const auto new_tile_coords = coords_of_tile_containing_player + Vec2i{chunk_x, chunk_y};
    //
    //             if(loaded_terrain_tiles.find(new_tile_coords) == nullptr) {
    //                 if(num_active_tilegen_tasks.load() < static_cast<Uint32>(cvar_max_generating_terrain_tiles->get())) {
    //                     logger->verbose("Marking tile (%d, %d) as having started loading", new_tile_coords.x, new_tile_coords.y);
    //                     loaded_terrain_tiles.insert(new_tile_coords, {});
    //                     num_active_tilegen_tasks.fetch_add(1);
    //                     ThreadPool::RunAsync([=](const IAsyncAction& /* work_item */) { generate_tile(new_tile_coords); });
    //                 }
    //             }
    //         }
    //     }
    // }
}

Float32 Terrain::get_terrain_height(const Vec2f& location) {
    const auto tilecoords = get_coords_of_tile_containing_position({location.x, 0, location.y});

    const auto tile_start_location = tilecoords * static_cast<Int32>(TILE_SIZE);
    const auto location_within_tile = Vec2u{static_cast<Uint32>(abs(round(location.x - tile_start_location.x))),
                                            static_cast<Uint32>(abs(round(location.y - tile_start_location.y)))};

    Rx::Concurrency::ScopeLock l{loaded_terrain_tiles_mutex};
    if(const auto* tile = loaded_terrain_tiles.find(tilecoords)) {
        if(tile->loading_phase != TerrainTile::LoadingPhase::GeneratingHeightmap) {
            return tile->heightmap[location_within_tile.y][location_within_tile.x];
        }
    }

    // Tile isn't loaded yet. Figure out how to handle this. Right now I don't want to deal with it, so I won't
    return 0;
}

Vec2i Terrain::get_coords_of_tile_containing_position(const Vec3f& position) {
    return Vec2i{static_cast<Int32>(round(position.x)), static_cast<Int32>(round(position.z))} / static_cast<Int32>(TILE_SIZE);
}

void Terrain::generate_heightmap(FastNoiseSIMD& noise_generator,
                                 const WorldParameters& params,
                                 renderer::Renderer& renderer,
                                 const com_ptr<ID3D12GraphicsCommandList4>& commands,
                                 TerrainData& data,
                                 const unsigned total_pixels_in_maps) {
    ZoneScoped;

    TracyD3D12Zone(renderer::RenderDevice::tracy_context, commands.get(), "Terrain::generate_heightmap");
    PIXScopedEvent(commands.get(), PIX_COLOR_DEFAULT, "Terrain::generate_heightmap");

    auto* height_noise = noise_generator.GetNoiseSet(-static_cast<Int32>(params.width) / 2,
                                                     -static_cast<Int32>(params.height) / 2,
                                                     0,
                                                     params.width,
                                                     params.height,
                                                     1);

    memcpy(data.heightmap.data(), height_noise, total_pixels_in_maps * sizeof(Float32));

    const auto min_terrain_height = params.min_terrain_depth_under_ocean;
    const auto max_terrain_height = params.min_terrain_depth_under_ocean + params.max_ocean_depth + params.max_height_above_sea_level;
    const auto height_range = max_terrain_height - min_terrain_height;

    data.heightmap.each_fwd([&](Float32& height) { height = height * height_range + min_terrain_height; });

    data.heightmap_handle = renderer.create_image({.name = "Terrain Heightmap",
                                                   .usage = renderer::ImageUsage::UnorderedAccess,
                                                   .format = renderer::ImageFormat::R32F,
                                                   .width = params.width,
                                                   .height = params.height},
                                                  data.heightmap.data(),
                                                  commands);
}

void Terrain::place_water_sources(const WorldParameters& params,
                                  renderer::Renderer& renderer,
                                  const com_ptr<ID3D12GraphicsCommandList4>& commands,
                                  TerrainData& data,
                                  const unsigned total_pixels_in_maps) {
    ZoneScoped;

    TracyD3D12Zone(renderer::RenderDevice::tracy_context, commands.get(), "Terrain::place_water_sources");
    PIXScopedEvent(commands.get(), PIX_COLOR_DEFAULT, "Terrain::place_water_sources");

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

    data.water_depth_handle = renderer.create_image({.name = "Terrain Water Map",
                                                     .usage = renderer::ImageUsage::UnorderedAccess,
                                                     .format = renderer::ImageFormat::Rg16F,
                                                     .width = params.width,
                                                     .height = params.height},
                                                    water_depth_map.data(),
                                                    commands);
}

void Terrain::compute_water_flow(renderer::Renderer& renderer, const com_ptr<ID3D12GraphicsCommandList4>& commands, TerrainData& data) {
    const auto& heightmap_image = renderer.get_image(data.heightmap_handle);
    const auto& watermap_image = renderer.get_image(data.water_depth_handle);
}

void Terrain::load_terrain_textures_and_create_material() {
    const char* albedo_texture_name = "data/textures/terrain/Ground_Forest_sfjmafua_8K_surface_ms/sfjmafua_512_Albedo.jpg";
    const char*
        normal_roughness_texture_name = "data/textures/terrain/Ground_Forest_sfjmafua_8K_surface_ms/sfjmafua_512_Normal_Roughness.jpg";

    ZoneScoped;

    auto material = renderer::StandardMaterial{};
    material.noise = renderer->get_noise_texture();

    const auto albedo_task = ThreadPool::RunAsync([&](const IAsyncAction& /* work_item */) {
        const auto albedo_image_handle = load_image_to_gpu(albedo_texture_name, *renderer);
        if(albedo_image_handle) {
            material.albedo = *albedo_image_handle;
        } else {
            logger->error("Could not load terrain albedo texture %s", albedo_texture_name);
            material.albedo = renderer->get_pink_texture();
        }
    });

    const auto normal_roughness_task = ThreadPool::RunAsync([&](const IAsyncAction& /* work_item */) {
        const auto normal_roughness_image_handle = load_image_to_gpu(normal_roughness_texture_name, *renderer);
        if(normal_roughness_image_handle) {
            material.normal_roughness = *normal_roughness_image_handle;
        } else {
            logger->error("Could not load terrain normal roughness texture %s", normal_roughness_texture_name);
            material.normal_roughness = renderer->get_default_normal_roughness_texture();
        }
    });

    albedo_task.get();
    normal_roughness_task.get();

    terrain_material = renderer->allocate_standard_material(material);
}

void Terrain::generate_tile(const Vec2i& tilecoord) {
    ZoneScoped;

    const auto top_left = tilecoord * static_cast<Int32>(TILE_SIZE);
    const auto size = Vec2u{TILE_SIZE, TILE_SIZE};

    logger->info("Generating tile (%d, %d) with size (%d, %d)", tilecoord.x, tilecoord.y, size.x, size.y);

    auto tile_heightmap = generate_terrain_heightmap(top_left, size);

    const auto tile_entity = registry->lock()->create();

    {
        Rx::Concurrency::ScopeLock l{loaded_terrain_tiles_mutex};
        auto* tile = loaded_terrain_tiles.find(tilecoord);
        tile->loading_phase = TerrainTile::LoadingPhase::GeneratingMesh;
        tile->heightmap = tile_heightmap;
        tile->coord = tilecoord;
        tile->entity = tile_entity;
    }

    logger->verbose("Finished generating heightmap for tile (%d, %d)", tilecoord.x, tilecoord.y);

    Rx::Vector<StandardVertex> tile_vertices;
    tile_vertices.reserve(tile_heightmap.size() * tile_heightmap[0].size());

    Rx::Vector<Uint32> tile_indices;
    tile_indices.reserve(tile_vertices.size() * 6);

    for(Uint32 y = 0; y < tile_heightmap.size(); y++) {
        const auto& tile_heightmap_row = tile_heightmap[y];
        for(Uint32 x = 0; x < tile_heightmap_row.size(); x++) {
            const auto height = tile_heightmap_row[x];

            const auto normal = get_normal_at_location(Vec2f{static_cast<Float32>(x), static_cast<Float32>(y)});

            tile_vertices.push_back(StandardVertex{.position = {static_cast<Float32>(x), height, static_cast<Float32>(y)},
                                                   .normal = normal,
                                                   .color = 0xFFFFFFFF,
                                                   .texcoord = {static_cast<Float32>(x), static_cast<Float32>(y)}});

            if(x < tile_heightmap_row.size() - 1 && y < tile_heightmap.size() - 1) {
                const auto width = static_cast<Uint32>(tile_heightmap_row.size());
                const auto face_start_idx = static_cast<Uint32>(y * width + x);

                // TODO: Triangulate the terrain mesh such that the vertices joined by an edge have more similar normals the the vertices
                // that don't share an edge

                tile_indices.push_back(face_start_idx);
                tile_indices.push_back(face_start_idx + 1);
                tile_indices.push_back(face_start_idx + width);

                tile_indices.push_back(face_start_idx + width);
                tile_indices.push_back(face_start_idx + 1);
                tile_indices.push_back(face_start_idx + width + 1);
            }
        }
    }

    {
        auto locked_tile_mesh_queue = tile_mesh_create_infos.lock();
        locked_tile_mesh_queue->emplace_back(tilecoord, tile_entity, Rx::Utility::move(tile_vertices), Rx::Utility::move(tile_indices));
    }

    logger->verbose("Finished generating mesh for tile (%d, %d)", tilecoord.x, tilecoord.y);

    num_active_tilegen_tasks.fetch_sub(1);
}

Rx::Vector<Rx::Vector<Float32>> Terrain::generate_terrain_heightmap(const Vec2i& top_left, const Vec2u& size) {
    const auto height_range = max_terrain_height - min_terrain_height;

    Rx::Vector<Rx::Vector<Float32>> heightmap;
    heightmap.reserve(size.x);

    auto raw_noise = Rx::Vector<Float32>{size.y * size.x};

    {
        Rx::Concurrency::ScopeLock l{noise_generator_mutex};
        noise_generator->FillNoiseSet(raw_noise.data(), top_left.x, top_left.y, 1, size.x, size.y, 1);
    }

    for(Uint32 y = 0; y < size.y; y++) {
        for(Uint32 x = 0; x < size.x; x++) {
            if(heightmap.size() <= y) {
                heightmap.emplace_back();
                heightmap[y].reserve(size.x);
            }
            if(heightmap[y].size() <= x) {
                heightmap[y].emplace_back();
            }
            heightmap[y][x] = raw_noise[y * size.x + x] * height_range + min_terrain_height;
        }
    }

    return heightmap;
}

void Terrain::upload_new_tile_meshes() {
    ZoneScoped;
    PIXScopedEvent(PIX_COLOR_DEFAULT, "Upload new terrain tile meshes");

    auto locked_tile_mesh_queue = tile_mesh_create_infos.lock();
    if(locked_tile_mesh_queue->is_empty()) {
        return;
    }

    auto& device = renderer->get_render_device();

    auto commands = device.create_command_list();
    commands->SetName(L"Terrain::upload_new_tile_meshes");
    {
        TracyD3D12Zone(renderer::RenderDevice::tracy_context, commands.get(), "Terrain::upload_new_tile_meshes");
        PIXScopedEvent(commands.get(), PIX_COLOR_DEFAULT, "Terrain::upload_new_tile_meshes");

        Rx::Vector<renderer::VisibleObjectCullingInformation> tile_culling_information{locked_tile_mesh_queue->size()};

        locked_tile_mesh_queue->each_fwd([&](const TerrainTileMeshCreateInfo& create_info) {
            PIXScopedEvent(commands.get(),
                           PIX_COLOR_DEFAULT,
                           "Terrain::upload_new_tile_meshes(%d, %d)",
                           create_info.tilecoord.x,
                           create_info.tilecoord.y);

            auto& meshes = renderer->get_static_mesh_store();

            meshes.begin_adding_meshes(commands.get());

            const auto tile_mesh_ld = meshes.add_mesh(create_info.vertices, create_info.indices, commands.get());
            const auto& vertex_buffer = *meshes.get_vertex_bindings()[0].buffer;

            float max_y = 0;
            float min_y = 256;

            create_info.vertices.each_fwd([&](const StandardVertex& vertex) {
                if(vertex.position.y < min_y) {
                    min_y = vertex.position.y;
                }

                if(vertex.position.y > max_y) {
                    max_y = vertex.position.y;
                }
            });

            const Rx::Vector<D3D12_RESOURCE_BARRIER>
                barriers = Rx::Array{CD3DX12_RESOURCE_BARRIER::Transition(vertex_buffer.resource.get(),
                                                                          D3D12_RESOURCE_STATE_COPY_DEST,
                                                                          D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                                     CD3DX12_RESOURCE_BARRIER::Transition(meshes.get_index_buffer().resource.get(),
                                                                          D3D12_RESOURCE_STATE_COPY_DEST,
                                                                          D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)};
            commands->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

            const auto ray_geo = renderer->create_raytracing_geometry(vertex_buffer,
                                                                      meshes.get_index_buffer(),
                                                                      Rx::Array{tile_mesh_ld},
                                                                      commands.get());
            const auto tile_mesh = tile_mesh_ld;

            renderer->add_raytracing_objects_to_scene(Rx::Array{renderer::RaytracingObject{.geometry_handle = ray_geo, .material = {0}}});

            {
                auto locked_registry = registry->lock();
                locked_registry->assign<renderer::StandardRenderableComponent>(create_info.entity, tile_mesh, terrain_material);
                locked_registry->assign<TransformComponent>(create_info.entity,
                                                            glm::vec3{create_info.tilecoord.x, 0.0f, create_info.tilecoord.y});
            }

            {
                logger->verbose("Marking tile (%d, %d) as completely loaded", create_info.tilecoord.x, create_info.tilecoord.y);
                Rx::Log::flush();
                Rx::Concurrency::ScopeLock l{loaded_terrain_tiles_mutex};
                loaded_terrain_tiles.find(create_info.tilecoord)->loading_phase = TerrainTile::LoadingPhase::Complete;
            }

            const auto cull_info = renderer::VisibleObjectCullingInformation{.aabb_x_min_max = {static_cast<float>(create_info.tilecoord.x),
                                                                                                static_cast<float>(create_info.tilecoord.x +
                                                                                                                   TILE_SIZE)},
                                                                             .aabb_y_min_max = {min_y, max_y},
                                                                             .aabb_z_min_max = {static_cast<float>(create_info.tilecoord.y),
                                                                                                static_cast<float>(create_info.tilecoord.y +
                                                                                                                   TILE_SIZE)},
                                                                             .vertex_count = tile_mesh_ld.num_vertices,
                                                                             .start_vertex_location = tile_mesh_ld.first_vertex};
            tile_culling_information.push_back(cull_info);
        });

        locked_tile_mesh_queue->clear();
    }
    {
        TracyD3D12Zone(renderer::RenderDevice::tracy_context, commands.get(), "Terrain::upload_new_tile_meshes::upload_visible_objects");
        PIXScopedEvent(commands.get(), PIX_COLOR_DEFAULT, "Terrain::upload_new_tile_meshes::upload_visible_objects");

        // TODO: Copy the staging buffer to the global scene cullable objects buffer
    }

    device.submit_command_list(Rx::Utility::move(commands));
}

Vec3f Terrain::get_normal_at_location(const Vec2f& location) {
    const auto height_middle_right = get_terrain_height(location + Vec2f{1, 0});
    const auto height_bottom_middle = get_terrain_height(location + Vec2f{0, -1});
    const auto height_top_middle = get_terrain_height(location + Vec2f{0, 1});
    const auto height_middle_left = get_terrain_height(location + Vec2f{-1, 0});

    const auto va = normalize(Vec3f{2.0, 0.0, height_middle_right - height_middle_left});
    const auto vb = normalize(Vec3f{0.0, 2.0, height_bottom_middle - height_top_middle});
    const auto normal = normalize(cross(va, vb));
    return {normal.x, normal.z, -normal.y};
}

Rx::Concurrency::Atomic<Uint32>& Terrain::get_num_active_tilegen_tasks() { return num_active_tilegen_tasks; }
