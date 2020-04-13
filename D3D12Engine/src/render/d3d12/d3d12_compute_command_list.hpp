#pragma once

#include "../compute_command_list.hpp"
#include "d3d12_compute_pipeline_state.hpp"
#include "d3d12_resource_command_list.hpp"

namespace render {
    class D3D12ComputeCommandList final : public D3D12ResourceCommandList, public virtual ComputeCommandList {
    public:
        D3D12ComputeCommandList(rx::memory::allocator& allocator, ComPtr<ID3D12GraphicsCommandList> cmds, D3D12RenderDevice& device_in);

#pragma region ComputeCommandList
        ~D3D12ComputeCommandList() override = default;

        void set_pipeline_state(const ComputePipelineState& state) override;

        void bind_compute_material(const Material& material) override;

        void dispatch(uint32_t workgroup_x, uint32_t workgroup_y, uint32_t workgroup_z) override;
#pragma endregion

    protected:
        const D3D12ComputePipelineState* compute_pipeline{nullptr};

        bool are_compute_resources_bound{false};
    };
} // namespace render
