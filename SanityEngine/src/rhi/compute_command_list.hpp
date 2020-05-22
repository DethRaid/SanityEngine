#pragma once

#include <stdint.h>

#include "mesh_data_store.hpp"
#include "raytracing_structs.hpp"
#include "resource_command_list.hpp"

namespace rhi {
    struct ComputePipelineState;
    struct BindGroup;

    /*!
     * \brief A command list which can execute compute tasks
     */
    class ComputeCommandList : public ResourceCommandList {
    public:
        ComputeCommandList(ComPtr<ID3D12GraphicsCommandList4> cmds, RenderDevice& device_in, ID3D12InfoQueue* info_queue_in);

        ComputeCommandList(const ComputeCommandList& other) = delete;
        ComputeCommandList& operator=(const ComputeCommandList& other) = delete;

        ComputeCommandList(ComputeCommandList&& old) noexcept;
        ComputeCommandList& operator=(ComputeCommandList&& old) noexcept;

        void bind_pipeline_state(const ComputePipelineState& state);

        void bind_compute_resources(const BindGroup& bind_group);

        void dispatch(uint32_t workgroup_x, uint32_t workgroup_y = 1, uint32_t workgroup_z = 1);

        void bind_mesh_data(const MeshDataStore& mesh_data);

        RaytracingMesh build_acceleration_structure_for_mesh(uint32_t num_vertices,
                                                             uint32_t num_indices,
                                                             uint32_t first_index);

        RaytracingMesh build_acceleration_structure_for_meshes(const std::vector<Mesh>& meshes);

        RaytracingScene build_raytracing_scene(const std::vector<RaytracingObject>& objects);

    protected:
        const ComputePipelineState* compute_pipeline{nullptr};

        ID3D12DescriptorHeap* current_descriptor_heap{nullptr};

        bool is_mesh_data_bound{false};
        const MeshDataStore* current_mesh_data{nullptr};

        bool are_compute_resources_bound{false};
    };

} // namespace rhi
