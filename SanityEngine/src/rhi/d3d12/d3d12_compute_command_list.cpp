#include "d3d12_compute_command_list.hpp"

#include <minitrace.h>
#include <spdlog/spdlog.h>

#include "../../core/ensure.hpp"
#include "../compute_pipeline_state.hpp"

using std::move;

namespace rhi {
    D3D12ComputeCommandList::D3D12ComputeCommandList(ComPtr<ID3D12GraphicsCommandList> cmds, D3D12RenderDevice& device_in)
        : D3D12ResourceCommandList{move(cmds), device_in} {}

    D3D12ComputeCommandList::D3D12ComputeCommandList(D3D12ComputeCommandList&& old) noexcept
        : D3D12ResourceCommandList(move(old)),
          compute_pipeline{old.compute_pipeline},
          are_compute_resources_bound{old.are_compute_resources_bound} {}

    D3D12ComputeCommandList& D3D12ComputeCommandList::operator=(D3D12ComputeCommandList&& old) noexcept {
        compute_pipeline = old.compute_pipeline;
        are_compute_resources_bound = old.are_compute_resources_bound;

        return static_cast<D3D12ComputeCommandList&>(D3D12ResourceCommandList::operator=(move(old)));
    }

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

    void D3D12ComputeCommandList::bind_compute_resources(const BindGroup& bind_group) {
        MTR_SCOPE("D3D12ComputeCommandList", "bind_compute_resources");

        ENSURE(compute_pipeline != nullptr, "Can not bind compute resources to a command list before you bind a compute pipeline");

        const auto& d3d12_bind_group = static_cast<const D3D12BindGroup&>(bind_group);

        for(const auto& image : d3d12_bind_group.used_images) {
            set_resource_state(*image.resource, image.states);
        }

        for(const auto& buffer : d3d12_bind_group.used_buffers) {
            set_resource_state(*buffer.resource, buffer.states);
        }

        d3d12_bind_group.bind_to_compute_signature(*commands.Get());

        are_compute_resources_bound = true;

        command_types.insert(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    }

    void D3D12ComputeCommandList::dispatch(const uint32_t workgroup_x, const uint32_t workgroup_y, const uint32_t workgroup_z) {
        MTR_SCOPE("D3D12ComputeCommandList", "dispatch");

        #ifndef NDEBUG
            ENSURE(compute_pipeline != nullptr, "Can not dispatch a compute workgroup before binding a compute pipeline");

            if(workgroup_x == 0) {
                spdlog::warn("Your workgroup has a width of 0. Are you sure you want to do that?");
            }

            if(workgroup_y == 0) {
                spdlog::warn("Your workgroup has a height of 0. Are you sure you want to do that?");
            }

            if(workgroup_z == 0) {
                spdlog::warn("Your workgroup has a depth of 0. Are you sure you want to do that?");
            }

            if(!are_compute_resources_bound) {
                // TODO: Disable this warning if it proves useless
                // TODO: Promote to an error if it proves useful
                spdlog::warn("Dispatching a compute job with no resource bound! Are you sure?");
            }
        #endif

        if(compute_pipeline != nullptr) {
            commands->Dispatch(workgroup_x, workgroup_y, workgroup_z);
        }

        command_types.insert(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    }
} // namespace render
