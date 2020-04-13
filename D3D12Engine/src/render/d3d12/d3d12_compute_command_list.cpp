#include "d3d12_compute_command_list.hpp"

#include <minitrace.h>
#include <rx/core/log.h>

#include "../compute_pipeline_state.hpp"
#include "../resource_binder.hpp"

#ifdef interface
#undef interface
#endif

using rx::utility::move;

namespace render {
    RX_LOG("D3D12ComputeCommandList", logger);

    D3D12ComputeCommandList::D3D12ComputeCommandList(rx::memory::allocator& allocator,
                                                     ComPtr<ID3D12GraphicsCommandList> cmds,
                                                     D3D12RenderDevice& device_in)
        : D3D12ResourceCommandList{allocator, move(cmds), device_in} {}

    void D3D12ComputeCommandList::set_pipeline_state(const ComputePipelineState& state) {
        MTR_SCOPE("D3D12ComputeCommandList", "set_pipeline_state");

        const auto& d3d12_state = static_cast<const D3D12ComputePipelineState&>(state);

        if(compute_pipeline == nullptr) {
            // First time binding a compute pipeline to this command list, we need to bind the root signature
            commands->SetComputeRootSignature(d3d12_state.root_signature.Get());
            are_compute_resources_bound = false;

        } else if(d3d12_state.root_signature != compute_pipeline->root_signature) {
            // Previous compute pipeline had a different root signature. Bind the new root signature
            commands->SetComputeRootSignature(d3d12_state.root_signature.Get());
            are_compute_resources_bound = false;
        }

        compute_pipeline = &d3d12_state;

        commands->SetPipelineState(d3d12_state.pso.Get());

        command_types.insert(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    }

    void D3D12ComputeCommandList::bind_compute_resources(Material& resources) {
        MTR_SCOPE("D3D12ComputeCommandList", "bind_compute_resources");

        if(should_do_validation && compute_pipeline == nullptr) {
            logger->error("Can not bind compute resources to a command list before you bind a compute pipeline");
        }

        are_compute_resources_bound = true;

        command_types.insert(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    }

    void D3D12ComputeCommandList::dispatch(const uint32_t workgroup_x, const uint32_t workgroup_y, const uint32_t workgroup_z) {
        MTR_SCOPE("D3D12ComputeCommandList", "dispatch");

        if(should_do_validation) {
            if(compute_pipeline == nullptr) {
                logger->error("Can not dispatch a compute workgroup before binding a compute pipeline");
            }

            if(workgroup_x == 0) {
                logger->warning("Your workgroup has a width of 0. Are you sure you want to do that?");
            }

            if(workgroup_y == 0) {
                logger->warning("Your workgroup has a height of 0. Are you sure you want to do that?");
            }

            if(workgroup_z == 0) {
                logger->warning("Your workgroup has a depth of 0. Are you sure you want to do that?");
            }
        }

        if(compute_pipeline != nullptr) {
            commands->Dispatch(workgroup_x, workgroup_y, workgroup_z);
        }

        command_types.insert(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    }
} // namespace render
