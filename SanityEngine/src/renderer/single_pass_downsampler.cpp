#include "single_pass_downsampler.hpp"

#include "loading/shader_loading.hpp"
#include "renderer/rhi/render_device.hpp"
#include "rhi/d3d12_private_data.hpp"
#include "rx/core/log.h"

#define A_CPU
#include "ffx_a.h"
#include "ffx_spd.h"

namespace sanity::engine::renderer {
    RX_LOG("SinglePassDenoiser", logger);

    constexpr auto SPD_MAX_MIP_LEVELS = 12;

    SinglePassDownsampler SinglePassDownsampler::Create(RenderBackend& backend) {
        Rx::Vector<CD3DX12_ROOT_PARAMETER> spd_params;
        spd_params.resize(3);

        // Shader parameter constants
        spd_params[ROOT_CONSTANTS_INDEX].InitAsConstants(6, 0);

        // UAV table + global counter buffer
        spd_params[GLOBAL_COUNTER_BUFFER_INDEX].InitAsUnorderedAccessView(1);

        D3D12_DESCRIPTOR_RANGE ranges[3] = {CD3DX12_DESCRIPTOR_RANGE{D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2},
                                            CD3DX12_DESCRIPTOR_RANGE{D3D12_DESCRIPTOR_RANGE_TYPE_UAV, SPD_MAX_MIP_LEVELS + 1, 3},
                                            CD3DX12_DESCRIPTOR_RANGE{D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0}};

        spd_params[DESCRIPTOR_TABLE_INDEX].InitAsDescriptorTable(3, ranges);

        auto static_sampler_desc = CD3DX12_STATIC_SAMPLER_DESC{0};
        static_sampler_desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        static_sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        static_sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        static_sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        static_sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        static_sampler_desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;

        const auto desc = D3D12_ROOT_SIGNATURE_DESC{.NumParameters = static_cast<UINT>(spd_params.size()),
                                                    .pParameters = spd_params.data(),
                                                    .NumStaticSamplers = 1,
                                                    .pStaticSamplers = &static_sampler_desc};

        const auto spd_root_sig = backend.compile_root_signature(desc);
        set_object_name(spd_root_sig.Get(), "SPD Root Signature");

        const auto compute_instructions = load_shader("utility/single_pass_downsampler.compute");

        const auto spd_pipeline = backend.create_compute_pipeline_state(compute_instructions, spd_root_sig);
        set_object_name(spd_pipeline.Get(), "SPD Compute Pipeline");

        return SinglePassDownsampler{spd_root_sig, spd_pipeline, backend};
    }

    void SinglePassDownsampler::generate_mip_chain_for_texture(ID3D12Resource* texture, ID3D12GraphicsCommandList4* cmds) {
        const auto device = backend->device;

        // Initialize SPD parameters
        varAU2(dispatch_thread_group_count_xy);
        varAU2(work_group_offset);
        varAU2(num_work_groups_and_mips);

        const auto desc = texture->GetDesc();
        varAU4(rectInfo) = initAU4(0, 0, static_cast<AU1>(desc.Width), desc.Height);
        const auto inverse_size = Vec2f{1.0f / static_cast<Float32>(desc.Width), 1.0f / static_cast<Float32>(desc.Height)};

        SpdSetup(dispatch_thread_group_count_xy, work_group_offset, num_work_groups_and_mips, rectInfo);

        const auto num_work_groups = num_work_groups_and_mips[0];
        const auto num_mips = num_work_groups_and_mips[1];

        // Set up descriptors
        const auto descriptor_table_handle = fill_descriptor_table(texture, device, num_mips);

        auto global_counter_buffer = backend->create_buffer({.name = "SPD Global Counter",
                                                             .usage = BufferUsage::UnorderedAccess,
                                                             .size = sizeof(Uint32)});

        // Bind descriptor heap, root signature, and pipeline
        auto* descriptor_heap = backend->get_cbv_srv_uav_heap();
        cmds->SetDescriptorHeaps(1, &descriptor_heap);
        cmds->SetComputeRootSignature(root_signature.Get());
        cmds->SetPipelineState(pipeline.Get());

        // Set parameters
        cmds->SetComputeRoot32BitConstant(ROOT_CONSTANTS_INDEX, num_mips, MIP_COUNT_ROOT_CONSTANT_OFFSET);
        cmds->SetComputeRoot32BitConstant(ROOT_CONSTANTS_INDEX, num_work_groups, NUM_WORK_GROUPS_ROOT_CONSTANT_OFFSET);
        cmds->SetComputeRoot32BitConstants(ROOT_CONSTANTS_INDEX, 2, &work_group_offset, OFFSET_ROOT_CONSTANT_OFFSET);
        cmds->SetComputeRoot32BitConstants(ROOT_CONSTANTS_INDEX, 2, &inverse_size, INVERSE_SIZE_ROOT_CONSTANT_OFFSET);

        cmds->SetComputeRootUnorderedAccessView(GLOBAL_COUNTER_BUFFER_INDEX, global_counter_buffer->resource->GetGPUVirtualAddress());

        cmds->SetComputeRootDescriptorTable(DESCRIPTOR_TABLE_INDEX, descriptor_table_handle.gpu_handle);

        // Set counter to 0
        {
            const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(global_counter_buffer->resource.Get(),
                                                                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                      D3D12_RESOURCE_STATE_COPY_DEST,
                                                                      0);
            cmds->ResourceBarrier(1, &barrier);

            D3D12_WRITEBUFFERIMMEDIATE_PARAMETER param{.Dest = global_counter_buffer->resource->GetGPUVirtualAddress(), .Value = 0};
            cmds->WriteBufferImmediate(1, &param, nullptr);

            D3D12_RESOURCE_BARRIER barriers[2] =
                {CD3DX12_RESOURCE_BARRIER::Transition(global_counter_buffer->resource.Get(),
                                                      D3D12_RESOURCE_STATE_COPY_DEST,
                                                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                      0),
                 CD3DX12_RESOURCE_BARRIER::Transition(texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)};
            cmds->ResourceBarrier(2, barriers);
        }

        cmds->Dispatch(dispatch_thread_group_count_xy[0], dispatch_thread_group_count_xy[1], 1);

        const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture,
                                                                  D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                  D3D12_RESOURCE_STATE_COPY_DEST);
        cmds->ResourceBarrier(1, &barrier);

        backend->schedule_buffer_destruction(Rx::Utility::move(global_counter_buffer));
    }

    SinglePassDownsampler::SinglePassDownsampler(ComPtr<ID3D12RootSignature> root_signature_in,
                                                 ComPtr<ID3D12PipelineState> pipeline_in,
                                                 RenderBackend& backend_in)
        : root_signature{Rx::Utility::move(root_signature_in)}, pipeline{Rx::Utility::move(pipeline_in)}, backend{&backend_in} {}

    DescriptorTableHandle SinglePassDownsampler::fill_descriptor_table(ID3D12Resource* texture,
                                                                       const ComPtr<ID3D12Device>& device,
                                                                       const Uint32 num_mips) const {
        const auto desc = texture->GetDesc();
        const auto descriptor_size = backend->get_shader_resource_descriptor_size();
        const auto output_mips_descriptors = backend->allocate_descriptor_table(num_mips - 1);

        auto descriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE{output_mips_descriptors.cpu_handle};

    	const auto mip_6_actual_slice = std::min(6u, num_mips);
        const auto mip_6_uav_desc = D3D12_UNORDERED_ACCESS_VIEW_DESC{.Format = desc.Format,
                                                                     .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
                                                                     .Texture2D = {.MipSlice = mip_6_actual_slice, .PlaneSlice = 0}};
        device->CreateUnorderedAccessView(texture, nullptr, &mip_6_uav_desc, descriptor);

        descriptor = descriptor.Offset(1, descriptor_size);

        for(Uint32 i = 0; i < num_mips; i++) {
            auto uav_desc = D3D12_UNORDERED_ACCESS_VIEW_DESC{.Format = desc.Format,
                                                             .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
                                                             .Texture2D = {.MipSlice = i + 1, .PlaneSlice = 0}};

            device->CreateUnorderedAccessView(texture, nullptr, &uav_desc, descriptor);

            descriptor = descriptor.Offset(1, descriptor_size);
        }

        const auto srv_desc = D3D12_SHADER_RESOURCE_VIEW_DESC{.Format = desc.Format,
                                                              .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
                                                              .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                                                              .Texture2D = {.MostDetailedMip = 0,
                                                                            .MipLevels = 1,
                                                                            .PlaneSlice = 0,
                                                                            .ResourceMinLODClamp = 0}};
        device->CreateShaderResourceView(texture, &srv_desc, descriptor);

        return output_mips_descriptors;
    }
} // namespace sanity::engine::renderer
