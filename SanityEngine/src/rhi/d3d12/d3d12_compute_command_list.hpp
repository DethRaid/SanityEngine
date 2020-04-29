#pragma once

#include "../compute_command_list.hpp"
#include "d3d12_compute_pipeline_state.hpp"
#include "d3d12_resource_command_list.hpp"

namespace rhi {
    class D3D12ComputeCommandList : public D3D12ResourceCommandList, public virtual ComputeCommandList {
    public:
        D3D12ComputeCommandList(ComPtr<ID3D12GraphicsCommandList> cmds, D3D12RenderDevice& device_in);

        D3D12ComputeCommandList(const D3D12ComputeCommandList& other) = delete;
        D3D12ComputeCommandList& operator=(const D3D12ComputeCommandList& other) = delete;

        D3D12ComputeCommandList(D3D12ComputeCommandList&& old) noexcept;
        D3D12ComputeCommandList& operator=(D3D12ComputeCommandList&& old) noexcept;

#pragma region ComputeCommandList
        ~D3D12ComputeCommandList() override = default;

        void set_pipeline_state(const ComputePipelineState& state) override;

        void bind_compute_resources(const BindGroup& bind_group) override;

        void dispatch(uint32_t workgroup_x, uint32_t workgroup_y, uint32_t workgroup_z) override;
#pragma endregion

    protected:
        const D3D12ComputePipelineState* compute_pipeline{nullptr};

        bool are_compute_resources_bound{false};
    };
} // namespace render
