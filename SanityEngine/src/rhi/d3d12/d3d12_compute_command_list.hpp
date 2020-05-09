#pragma once

#include "../compute_command_list.hpp"
#include "d3d12_compute_pipeline_state.hpp"
#include "d3d12_resource_command_list.hpp"

namespace rhi {
    class D3D12ComputeCommandList : public D3D12ResourceCommandList, public virtual ComputeCommandList {
    public:
        D3D12ComputeCommandList(ComPtr<ID3D12GraphicsCommandList4> cmds, D3D12RenderDevice& device_in);

        D3D12ComputeCommandList(const D3D12ComputeCommandList& other) = delete;
        D3D12ComputeCommandList& operator=(const D3D12ComputeCommandList& other) = delete;

        D3D12ComputeCommandList(D3D12ComputeCommandList&& old) noexcept;
        D3D12ComputeCommandList& operator=(D3D12ComputeCommandList&& old) noexcept;

#pragma region ComputeCommandList
        ~D3D12ComputeCommandList() override = default;

        void set_pipeline_state(const ComputePipelineState& state) override;

        void bind_compute_resources(const BindGroup& bind_group) override;

        void dispatch(uint32_t workgroup_x, uint32_t workgroup_y, uint32_t workgroup_z) override;

        void bind_mesh_data(const MeshDataStore& mesh_data) override;

        RaytracingMesh build_acceleration_structure_for_mesh(uint32_t num_vertices,
                                                             uint32_t num_indices,
                                                             uint32_t first_vertex,
                                                             uint32_t first_index) override;

        RaytracingMesh build_acceleration_structure_for_meshes(const std::vector<renderer::Mesh>& meshes) override;

        RaytracingScene build_raytracing_scene(const std::vector<RaytracingObject>& objects) override;
#pragma endregion

    protected:
        const D3D12ComputePipelineState* compute_pipeline{nullptr};

        ID3D12DescriptorHeap* current_descriptor_heap{nullptr};

        const MeshDataStore* current_mesh_data{nullptr};

        bool are_compute_resources_bound{false};
    };
} // namespace render
