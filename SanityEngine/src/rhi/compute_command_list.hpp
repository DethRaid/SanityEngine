#pragma once

#include <stdint.h>

#include "../renderer/mesh.hpp"
#include "raytracing_structs.hpp"
#include "resource_command_list.hpp"

namespace rhi {
    class MeshDataStore;
    struct ComputePipelineState;
    struct BindGroup;

    /*!
     * \brief A command list which can execute compute tasks
     */
    class ComputeCommandList : virtual public ResourceCommandList {
    public:
        /*!
         * \brief Sets the state of the compute pipeline
         *
         * MUST be called before `bind` or `dispatch`
         */
        virtual void set_pipeline_state(const ComputePipelineState& state) = 0;

        /*!
         * \brief Binds resources for use by compute dispatches
         *
         * MUST be called after `set_pipeline_state`
         */
        virtual void bind_compute_resources(const BindGroup& resources) = 0;

        /*!
         * \brief Dispatches a compute workgroup to perform some work
         *
         * MUST be called after set_pipeline_state
         */
        virtual void dispatch(uint32_t workgroup_x, uint32_t workgroup_y = 1, uint32_t workgroup_z = 1) = 0;

        /*!
         * \brief Binds the provided mesh data to the command list. Subsequent drawcalls will render this mesh data, until you bind new mesh
         * data
         */
        virtual void bind_mesh_data(const MeshDataStore& mesh_data) = 0;

        /*!
         * \brief Builds an acceleration structure for the mesh with the provided parameters
         *
         * \param num_vertices Number of vertices in the mesh
         * \param num_indices Number of vertices in the mesh
         * \param first_vertex The first vertex that the mesh uses
         * \param first_index The first index that the mesh uses
         */
        [[nodiscard]] virtual RaytracingMesh build_acceleration_structure_for_mesh(uint32_t num_vertices,
                                                                                   uint32_t num_indices,
                                                                                   uint32_t first_vertex = 0,
                                                                                   uint32_t first_index = 0) = 0;

        /*!
         * \brief Builds an acceleration structure for a lot of meshes, all at once
         */
        [[nodiscard]] virtual RaytracingMesh build_acceleration_structure_for_meshes(const std::vector<renderer::Mesh>& meshes) = 0;

        /*!
         * \brief Creates a raytracing scene that will contain all the provided objects
         */
        [[nodiscard]] virtual RaytracingScene build_raytracing_scene(const std::vector<RaytracingObject>& objects) = 0;
    };

} // namespace rhi
