#include "gpu_terrain_generation.hpp"

#include "Tracy.hpp"
#include "TracyD3D12.hpp"
#include "loading/shader_loading.hpp"
#include "renderer/renderer.hpp"
#include "renderer/rhi/d3d12_private_data.hpp"
#include "renderer/rhi/render_device.hpp"
#include "world/terrain.hpp"

namespace sanity::engine::terraingen {
    RX_LOG("Terraingen", logger);

    RX_CONSOLE_IVAR(cvar_num_water_iterations,
                    "t.NumWaterFlowIterations",
                    "How many iterations of the wasic water flow simulations to perform",
                    1,
                    128,
                    16);

    static ComPtr<ID3D12PipelineState> place_oceans_pso;
    static ComPtr<ID3D12PipelineState> water_flow_pso;

    static void create_place_ocean_pipeline(renderer::RenderBackend& device) {
        ZoneScoped;

        const auto place_oceans_shader_source = load_shader("FillOcean.compute");
        RX_ASSERT(!place_oceans_shader_source.is_empty(), "Could not load shader FillOcean.compute");

        const auto descriptor_ranges = Rx::Array{D3D12_DESCRIPTOR_RANGE{.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV, .NumDescriptors = 1},
                                                 D3D12_DESCRIPTOR_RANGE{.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV, .NumDescriptors = 1}};

        const auto root_parameters = Rx::Array{D3D12_ROOT_PARAMETER{.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
                                                                    .Constants =
                                                                        {
                                                                            .Num32BitValues = 1,
                                                                        }},
                                               D3D12_ROOT_PARAMETER{.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                                                                    .DescriptorTable = {.NumDescriptorRanges = static_cast<UINT>(
                                                                                            descriptor_ranges.size()),
                                                                                        .pDescriptorRanges = descriptor_ranges.data()}}};

        const auto root_signature = device.compile_root_signature(
            {.NumParameters = static_cast<UINT>(root_parameters.size()), .pParameters = root_parameters.data()});
        place_oceans_pso = device.create_compute_pipeline_state(place_oceans_shader_source, root_signature);

        const auto table = device.allocate_descriptor_table(static_cast<UINT>(descriptor_ranges.size()));
        place_oceans_pso->SetPrivateData(PRIVATE_DATA_ATTRIBS(renderer::DescriptorTableHandle), &table);
    }

    static void create_water_flow_pipeline(renderer::RenderBackend& device) {
        ZoneScoped;

        const auto descriptor_ranges = Rx::Array{D3D12_DESCRIPTOR_RANGE{.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV, .NumDescriptors = 1},
                                                 D3D12_DESCRIPTOR_RANGE{.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV, .NumDescriptors = 1}};

        const auto root_parameters = Rx::Array{
            D3D12_ROOT_PARAMETER{.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                                 .DescriptorTable = {.NumDescriptorRanges = static_cast<UINT>(descriptor_ranges.size()),
                                                     .pDescriptorRanges = descriptor_ranges.data()}}};

        const auto root_signature = device.compile_root_signature(
            {.NumParameters = static_cast<UINT>(root_parameters.size()), .pParameters = root_parameters.data()});

        const auto water_flow_shader_source = load_shader("WaterFlow.compute");
        RX_ASSERT(!water_flow_shader_source.is_empty(), "Could not load shader WaterFlow.compute");

        water_flow_pso = device.create_compute_pipeline_state(water_flow_shader_source, root_signature);

        const auto table = device.allocate_descriptor_table(static_cast<UINT>(descriptor_ranges.size()));
        water_flow_pso->SetPrivateData(PRIVATE_DATA_ATTRIBS(renderer::DescriptorTableHandle), &table);
    }

    void initialize(renderer::RenderBackend& device) {
        ZoneScoped;

        create_place_ocean_pipeline(device);
        create_water_flow_pipeline(device);
    }

    void place_oceans(const ComPtr<ID3D12GraphicsCommandList4>& commands,
                      renderer::Renderer& renderer,
                      const Uint32 sea_level,
                      TerrainData& data) {
        ZoneScoped;

        TracyD3D12Zone(renderer::RenderBackend::tracy_context, commands.Get(), "gpu_terrain_generation::place_oceans");
        PIXScopedEvent(commands.Get(), PIX_COLOR_DEFAULT, "gpu_terrain_generation::place_oceans");

        data.water_depth_handle = renderer.create_image({
            .name = "Terrain water depth map",
            .usage = renderer::ImageUsage::UnorderedAccess,
            .format = renderer::ImageFormat::R32F,
            .width = data.size.max_longitude * 2,
            .height = data.size.max_latitude * 2,
        });

        const auto land_heightmap = renderer.get_image(data.heightmap_handle);
        const auto water_heightmap = renderer.get_image(data.water_depth_handle);

        auto& device = renderer.get_render_backend();
        auto d3d12_device = device.device;
        const auto descriptor_size = device.get_shader_resource_descriptor_size();

        auto descriptor_table = renderer::retrieve_object<renderer::DescriptorTableHandle>(place_oceans_pso.Get());

        const auto heightmap_desc = land_heightmap.resource->GetDesc();
        const auto heightmap_uav_desc = D3D12_SHADER_RESOURCE_VIEW_DESC{.Format = heightmap_desc.Format,
                                                                        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
                                                                        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                                                                        .Texture2D = {
                                                                            .MostDetailedMip = 0,
                                                                            .MipLevels = 0xFFFFFFFF,
                                                                        }};
        d3d12_device->CreateShaderResourceView(land_heightmap.resource.Get(), &heightmap_uav_desc, descriptor_table.cpu_handle);

        const auto water_height_map_desc = water_heightmap.resource->GetDesc();
        const auto water_height_map_uav_desc = D3D12_UNORDERED_ACCESS_VIEW_DESC{.Format = water_height_map_desc.Format,
                                                                                .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
                                                                                .Texture2D = {}};
        d3d12_device->CreateUnorderedAccessView(water_heightmap.resource.Get(),
                                                nullptr,
                                                &water_height_map_uav_desc,
                                                descriptor_table.cpu_handle.Offset(descriptor_size));

        const auto water_depth_map_barrier = CD3DX12_RESOURCE_BARRIER::UAV(water_heightmap.resource.Get());
        commands->ResourceBarrier(1, &water_depth_map_barrier);

        auto* heap = device.get_cbv_srv_uav_heap();
        commands->SetDescriptorHeaps(1, &heap);

        const auto root_sig = renderer::get_com_interface<ID3D12RootSignature>(place_oceans_pso.Get());
        commands->SetComputeRootSignature(root_sig.Get());
        commands->SetComputeRoot32BitConstant(0, sea_level, 0);
        commands->SetComputeRootDescriptorTable(1, descriptor_table.gpu_handle);

        commands->SetPipelineState(place_oceans_pso.Get());

        const auto desc = land_heightmap.resource->GetDesc();
        const auto thread_group_count_x = static_cast<Uint32>(desc.Width / 8);
        const auto thread_group_count_y = static_cast<Uint32>(desc.Height / 8);
        commands->Dispatch(thread_group_count_x, thread_group_count_y, 1);

        logger->info("Recorded place oceans compute shader dispatches");
    }

    void compute_water_flow(const ComPtr<ID3D12GraphicsCommandList4>& commands, renderer::Renderer& renderer, TerrainData& data) {
        ZoneScoped;

        TracyD3D12Zone(renderer::RenderBackend::tracy_context, commands.Get(), "gpu_terrain_generation::compute_water_flow");
        PIXScopedEvent(commands.Get(), PIX_COLOR_DEFAULT, "gpu_terrain_generation::compute_water_flow");

        const auto land_heightmap = renderer.get_image(data.heightmap_handle);
        const auto water_heightmap = renderer.get_image(data.water_depth_handle);
        auto& device = renderer.get_render_backend();
        auto d3d12_device = device.device;
        const auto descriptor_size = device.get_shader_resource_descriptor_size();

        auto descriptor_table = renderer::retrieve_object<renderer::DescriptorTableHandle>(place_oceans_pso.Get());

        const auto heightmap_desc = land_heightmap.resource->GetDesc();
        const auto heightmap_uav_desc = D3D12_SHADER_RESOURCE_VIEW_DESC{.Format = heightmap_desc.Format,
                                                                        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
                                                                        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                                                                        .Texture2D = {
                                                                            .MostDetailedMip = 0,
                                                                            .MipLevels = 0xFFFFFFFF,
                                                                        }};
        d3d12_device->CreateShaderResourceView(land_heightmap.resource.Get(), &heightmap_uav_desc, descriptor_table.cpu_handle);

        const auto water_height_map_desc = water_heightmap.resource->GetDesc();
        const auto water_height_map_uav_desc = D3D12_UNORDERED_ACCESS_VIEW_DESC{.Format = water_height_map_desc.Format,
                                                                                .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
                                                                                .Texture2D = {}};
        d3d12_device->CreateUnorderedAccessView(water_heightmap.resource.Get(),
                                                nullptr,
                                                &water_height_map_uav_desc,
                                                descriptor_table.cpu_handle.Offset(descriptor_size));

        const auto root_signature = renderer::get_com_interface<ID3D12RootSignature>(water_flow_pso.Get());
        commands->SetComputeRootSignature(root_signature.Get());
        commands->SetComputeRootDescriptorTable(0, descriptor_table.gpu_handle);

        commands->SetPipelineState(water_flow_pso.Get());

        const auto desc = land_heightmap.resource->GetDesc();
        const auto thread_group_count_x = static_cast<Uint32>(desc.Width / 8);
        const auto thread_group_count_y = static_cast<Uint32>(desc.Height / 8);

        const auto water_depth_map_barrier = CD3DX12_RESOURCE_BARRIER::UAV(water_heightmap.resource.Get());

        for(Uint32 water_flow_iteration = 0; water_flow_iteration < static_cast<Uint32>(cvar_num_water_iterations->get());
            water_flow_iteration++) {
            commands->ResourceBarrier(1, &water_depth_map_barrier);
            commands->Dispatch(thread_group_count_x, thread_group_count_y, 1);
        }

        logger->info("Recorded water flow compute shader dispatches");
    }
} // namespace terraingen
