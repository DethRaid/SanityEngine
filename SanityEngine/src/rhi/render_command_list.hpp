#pragma once

#include <optional>

#include "compute_command_list.hpp"
#include "framebuffer.hpp"
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
         * \brief Sets the material index for subsequent drawcalls to use
         */
        virtual void set_material_idx(uint32_t idx) = 0;

        /*!
         * \brief Draws some of the indices in the current mesh data
         *
         * This method MUST be called after bind_mesh_data
         */
        virtual void draw(uint32_t num_indices, uint32_t first_index = 0, uint32_t num_instances = 1) = 0;
    };
} // namespace rhi
