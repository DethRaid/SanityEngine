#pragma once

#include <optional>

#include "compute_command_list.hpp"
#include "framebuffer.hpp"
#include "raytracing_structs.hpp"
#include "resources.hpp"

namespace rhi {
    struct RenderPipelineState;
    struct Framebuffer;
    class MeshDataStore;

    class RenderCommandList : public virtual ComputeCommandList {
    public:
        using ComputeCommandList::set_pipeline_state;

        /*!
         * \brief Sets the render targets that draws will render to
         */
        virtual void set_framebuffer(const Framebuffer& framebuffer,
                                     std::vector<RenderTargetAccess> render_target_accesses,
                                     std::optional<RenderTargetAccess> depth_access = std::nullopt) = 0;

        /*!
         * \brief Sets the state of the graphics rendering pipeline
         */
        virtual void set_pipeline_state(const RenderPipelineState& state) = 0;

        /*!
         * \brief Sets the resources that rendering commands will use
         */
        virtual void bind_render_resources(const BindGroup& resources) = 0;

        /*!
         * \brief Sets the index of the camera to render with
         *
         * MUST be called after set_pipeline_state because of how the D3D12 backend works
         */
        virtual void set_camera_idx(uint32_t camera_idx) = 0;

        /*!
         * \brief Binds the provided mesh data to the command list. Subsequent drawcalls will render this mesh data, until you bind new mesh
         * data
         */
        virtual void bind_mesh_data(const MeshDataStore& mesh_data) = 0;

        /*!
         * \brief Sets the material index for subsequent drawcalls to use
         */
        virtual void set_material_idx(uint32_t idx) = 0;

        /*!
         * \brief Draws some of the indices in the current mesh data
         *
         * This method MUST be called after bind_mesh_data
         */
        virtual void draw(uint32_t num_indices, uint32_t first_index = 0, uint32_t num_instances = 1) = 0;

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
         * \brief Creates a raytracing scene that will contain all the provided objects
         */
        [[nodiscard]] virtual RaytracingScene build_raytracing_scene(const std::vector<RaytracingObject>& objects) = 0;
    };
} // namespace rhi
