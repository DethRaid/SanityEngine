#include "terrain.hpp"

#include <Tracy.hpp>
#include <entt/entity/registry.hpp>
#include <ftl/atomic_counter.h>
#include <ftl/task.h>
#include <rx/core/array.h>
#include <rx/core/log.h>

#include "loading/image_loading.hpp"
#include "renderer/standard_material.hpp"
#include "rhi/helpers.hpp"
#include "rhi/render_device.hpp"
#include "sanity_engine.hpp"

RX_LOG("Terrain", logger);

Terrain::Terrain(const Uint32 max_latitude_in,
                 const Uint32 max_longitude_in,
                 const Uint32 min_terrain_height_in,
                 const Uint32 max_terrain_height_in,
                 renderer::Renderer& renderer_in,
                 FastNoiseSIMD& noise_generator_in,
                 entt::registry& registry_in)
    : renderer{&renderer_in},
      noise_generator{&noise_generator_in},
      registry{&registry_in},
      max_latitude{max_latitude_in},
      max_longitude{max_longitude_in},
      min_terrain_height{min_terrain_height_in},
      max_terrain_height{max_terrain_height_in} {

    // TODO: Make a good data structure to load the terrain material(s) at runtime
    load_terrain_textures_and_create_material();
}

void Terrain::load_terrain_around_player(const TransformComponent& player_transform) {
    ZoneScopedN("load_terrain_around_player");
    const auto coords_of_tile_containing_player = get_coords_of_tile_containing_position(
        {player_transform.location.x, player_transform.location.y, player_transform.location.z});

    // V0: load the tile the player is in and nothing else

    // V1: load the tiles in the player's frustum, plus a few on either side so it's nice and fast for the player to spin around

    // TODO: Define some maximum number of tiles that may be loaded/generated in a given frame
    // Also TODO: Generate terrain tiles in in separate threads as part of the tasking system
    if(loaded_terrain_tiles.find(coords_of_tile_containing_player) == nullptr) {
        generate_tile(coords_of_tile_containing_player);
    }
}

Float32 Terrain::get_terrain_height(const Vec2f& location) const {
    const auto tilecoords = get_coords_of_tile_containing_position({location.x, 0, location.y});

    const auto tile_start_location = tilecoords * TILE_SIZE;
    const auto location_within_tile = Vec2u{static_cast<Uint32>(abs(round(location.x - tile_start_location.x))),
                                            static_cast<Uint32>(abs(round(location.y - tile_start_location.y)))};

    if(const auto* tile = loaded_terrain_tiles.find(tilecoords)) {
        return tile->heightmap[location_within_tile.y][location_within_tile.x];

    } else {
        // Tile isn't loaded yet. Figure out how to handle this. Right now I don't want to deal with it, so I won't
        return 0;
    }
}

Vec2i Terrain::get_coords_of_tile_containing_position(const Vec3f& position) {
    return Vec2i{static_cast<Int32>(round(position.x)), static_cast<Int32>(round(position.z))} / TILE_SIZE;
}

void Terrain::load_terrain_textures_and_create_material() {
    auto* sanity_engine_global = Rx::Globals::find("SanityEngine");
    auto* task_scheduler = sanity_engine_global->find("TaskScheduler")->cast<ftl::TaskScheduler>();

    ftl::AtomicCounter counter{task_scheduler};

    Rx::Ptr<LoadImageToGpuArgs> albedo_image_data = Rx::make_ptr<LoadImageToGpuArgs>(Rx::Memory::SystemAllocator::instance());
    albedo_image_data->texture_name_in = "data/textures/terrain/Ground_Forest_sfjmafua_8K_surface_ms/sfjmafua_512_Albedo.jpg";
    albedo_image_data->renderer_in = renderer;

    task_scheduler->AddTask({load_image_to_gpu, albedo_image_data.get()}, &counter);

    Rx::Ptr<LoadImageToGpuArgs> normal_roughness_image_data = Rx::make_ptr<LoadImageToGpuArgs>(Rx::Memory::SystemAllocator::instance());
    normal_roughness_image_data
        ->texture_name_in = "data/textures/terrain/Ground_Forest_sfjmafua_8K_surface_ms/sfjmafua_512_Normal_Roughness.jpg";
    normal_roughness_image_data->renderer_in = renderer;

    task_scheduler->AddTask({load_image_to_gpu, normal_roughness_image_data.get()}, &counter);

    auto material = renderer::StandardMaterial{};
    material.noise = renderer->get_noise_texture();

    task_scheduler->WaitForCounter(&counter, 0, true);

    if(albedo_image_data->handle_out) {
        material.albedo = *albedo_image_data->handle_out;
    } else {
        logger->error("Could not load terrain albedo texture %s", albedo_image_data->texture_name_in);
        material.albedo = renderer->get_pink_texture();
    }

    if(normal_roughness_image_data->handle_out) {
        material.normal_roughness = *normal_roughness_image_data->handle_out;
    } else {
        logger->error("Could not load terrain normal roughness texture %s", normal_roughness_image_data->texture_name_in);
        material.normal_roughness = renderer->get_default_normal_roughness_texture();
    }

    terrain_material = renderer->allocate_standard_material(material);
}

void Terrain::generate_tile(const Vec2i& tilecoord) {
    const auto top_left = tilecoord * TILE_SIZE;
    const auto size = Vec2u{TILE_SIZE, TILE_SIZE};

    logger->info("Generating tile (%d, %d) with size (%d, %d)", tilecoord.x, tilecoord.y, size.x, size.y);

    const auto tile_heightmap = generate_terrain_heightmap(top_left, size);

    const auto tile_entity = registry->create();

    loaded_terrain_tiles.insert(tilecoord, TerrainTile{tile_heightmap, tilecoord, tile_entity});

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

                tile_indices.push_back(face_start_idx);
                tile_indices.push_back(face_start_idx + 1);
                tile_indices.push_back(face_start_idx + width);

                tile_indices.push_back(face_start_idx + width);
                tile_indices.push_back(face_start_idx + 1);
                tile_indices.push_back(face_start_idx + width + 1);
            }
        }
    }

    auto& device = renderer->get_render_device();
    const auto commands = device.create_command_list(0);
    TracyD3D12Zone(rhi::RenderDevice::tracy_context, commands.Get(), "UploadTerrainTileMeshes");

    auto& meshes = renderer->get_static_mesh_store();

    meshes.begin_adding_meshes(commands);

    const auto tile_mesh = meshes.add_mesh(tile_vertices, tile_indices, commands);
    const auto& vertex_buffer = *meshes.get_vertex_bindings()[0].buffer;

    {
        const Rx::Vector<D3D12_RESOURCE_BARRIER>
            barriers = Rx::Array{CD3DX12_RESOURCE_BARRIER::Transition(vertex_buffer.resource.Get(),
                                                                      D3D12_RESOURCE_STATE_COPY_DEST,
                                                                      D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
                                 CD3DX12_RESOURCE_BARRIER::Transition(meshes.get_index_buffer().resource.Get(),
                                                                      D3D12_RESOURCE_STATE_COPY_DEST,
                                                                      D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)};
        commands->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    }

    const auto ray_geo = renderer->create_raytracing_geometry(vertex_buffer, meshes.get_index_buffer(), Rx::Array{tile_mesh}, commands);

    device.submit_command_list(commands);

    renderer->add_raytracing_objects_to_scene(Rx::Array{rhi::RaytracingObject{.geometry_handle = ray_geo, .material = {0}}});

    registry->assign<renderer::StandardRenderableComponent>(tile_entity, tile_mesh, terrain_material);
}

Rx::Vector<Rx::Vector<Float32>> Terrain::generate_terrain_heightmap(const Vec2i& top_left, const Vec2u& size) const {
    const auto height_range = max_terrain_height - min_terrain_height;

    Rx::Vector<Rx::Vector<Float32>> heightmap;

    auto raw_noise = Rx::Vector<Float32>{size.y * size.x};

    noise_generator->FillNoiseSet(raw_noise.data(), top_left.x, top_left.y, 1, size.x, size.y, 1);

    for(Uint32 y = 0; y < size.y; y++) {
        for(Uint32 x = 0; x < size.x; x++) {
            heightmap[y][x] = raw_noise[y * size.x + x] * height_range + min_terrain_height;
        }
    }

    return heightmap;
}

Vec3f Terrain::get_normal_at_location(const Vec2f& location) const {
    const auto height_middle_right = get_terrain_height(location + Vec2f{1, 0});
    const auto height_bottom_middle = get_terrain_height(location + Vec2f{0, -1});
    const auto height_top_middle = get_terrain_height(location + Vec2f{0, 1});
    const auto height_middle_left = get_terrain_height(location + Vec2f{-1, 0});

    const auto va = normalize(Vec3f{2.0, 0.0, height_middle_right - height_middle_left});
    const auto vb = normalize(Vec3f{0.0, 2.0, height_bottom_middle - height_top_middle});
    const auto normal = normalize(cross(va, vb));
    return {normal.x, normal.z, -normal.y};
}
