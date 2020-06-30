#include "gpu_terrain_generation.hpp"

#include <Tracy.hpp>
#include <TracyD3D12.hpp>

#include "loading/shader_loading.hpp"
#include "renderer/rhi/d3d12_private_data.hpp"
#include "rhi/render_device.hpp"

namespace terraingen {
    static ComPtr<ID3D12PipelineState> place_oceans_pso;

    void create_place_ocean_pso(renderer::RenderDevice& device) {
        const auto place_oceans_shader_source = load_shader("FillOcean.hlsl");

        const auto root_parameters = Rx::Array{D3D12_ROOT_PARAMETER{.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
                                                                    .Constants =
                                                                        {
                                                                            .Num32BitValues = 1,
                                                                        }},
                                               D3D12_ROOT_PARAMETER{.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV, .Descriptor = {}}};

        const auto root_signature = device.compile_root_signature(
            {.NumParameters = static_cast<UINT>(root_parameters.size()), .pParameters = root_parameters.data()});
        place_oceans_pso = device.create_compute_pipeline_state(place_oceans_shader_source, root_signature);
    }

    void create_pipelines(renderer::RenderDevice& device) { create_place_ocean_pso(device); }

    void place_oceans(const ComPtr<ID3D12GraphicsCommandList4>& cmds,
                      const ComPtr<ID3D12Resource>& height_water_map,
                      const float sea_level) {
        ZoneScoped;

        const auto* sea_level_uint = reinterpret_cast<const UINT*>(&sea_level);

        TracyD3D12Zone(renderer::RenderDevice::tracy_context, cmds.Get(), "gpu_terrain_generation::place_oceans");
        PIXScopedEvent(cmds.Get(), PIX_COLOR_DEFAULT, "gpu_terrain_generation::place_oceans");

        const auto root_sig = renderer::get_com_interface<ID3D12RootSignature>(place_oceans_pso.Get());
        cmds->SetComputeRootSignature(root_sig.Get());
        cmds->SetComputeRoot32BitConstant(0, *sea_level_uint, 0);
        cmds->SetComputeRootUnorderedAccessView(1, height_water_map->GetGPUVirtualAddress());

        cmds->SetPipelineState(place_oceans_pso.Get());

        const auto desc = height_water_map->GetDesc();
        const auto thread_group_count_x = desc.Width / 8;
        const auto thread_group_count_y = desc.Height / 8;
        cmds->Dispatch(thread_group_count_x, thread_group_count_y, 1);
    }
} // namespace terraingen
