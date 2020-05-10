#pragma once

#include <optional>

#include "compute_command_list.hpp"
#include "framebuffer.hpp"

namespace rhi {
    struct RenderPipelineState;
    struct Framebuffer;
    class MeshDataStore;

    class RenderCommandList : public ComputeCommandList {
    public:
        RenderCommandList(ComPtr<ID3D12GraphicsCommandList4> cmds, RenderDevice& device_in);

        RenderCommandList(const RenderCommandList& other) = delete;
        RenderCommandList& operator=(const RenderCommandList& other) = delete;

        RenderCommandList(RenderCommandList&& old) noexcept;
        RenderCommandList& operator=(RenderCommandList&& old) noexcept;

        using ComputeCommandList::set_pipeline_state;

        void set_framebuffer(const Framebuffer& framebuffer,
                             std::vector<RenderTargetAccess> render_target_accesses,
                             std::optional<RenderTargetAccess> depth_access);

        void set_pipeline_state(const RenderPipelineState& state);

        void bind_render_resources(const BindGroup& bind_group);

        void set_camera_idx(uint32_t camera_idx);

        void set_material_idx(uint32_t idx);

        void draw(uint32_t num_indices, uint32_t first_index, uint32_t num_instances);

        /*!
         * \brief Preforms all the necessary tasks to prepare this command list for submission to the GPU, including ending any pending
         * render passes
         */
        void prepare_for_submission();

    protected:
        bool in_render_pass{false};

        const RenderPipelineState* current_render_pipeline_state{nullptr};

        bool is_render_material_bound{false};
    };
} // namespace rhi
