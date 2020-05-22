#pragma once

#include <optional>

#include "compute_command_list.hpp"
#include "framebuffer.hpp"
#include "imgui/imgui.h"

namespace rhi {
    struct RenderPipelineState;
    struct Framebuffer;
    class MeshDataStore;

    class RenderCommandList : public ComputeCommandList {
    public:
        RenderCommandList(ComPtr<ID3D12GraphicsCommandList4> cmds, RenderDevice& device_in, ID3D12InfoQueue* info_queue_in);

        RenderCommandList(const RenderCommandList& other) = delete;
        RenderCommandList& operator=(const RenderCommandList& other) = delete;

        RenderCommandList(RenderCommandList&& old) noexcept;
        RenderCommandList& operator=(RenderCommandList&& old) noexcept;

        using ComputeCommandList::bind_pipeline_state;

        void begin_render_pass(const Framebuffer& framebuffer,
                               std::vector<RenderTargetAccess> render_target_accesses,
                               std::optional<RenderTargetAccess> depth_access = std::nullopt);

        void end_render_pass();

        void bind_ui_mesh(const Buffer& vertex_buffer, const Buffer& index_buffer);

        void bind_pipeline_state(const RenderPipelineState& state);

        void set_viewport(const glm::vec2& display_pos, const glm::vec2& display_size) const;

        void set_scissor_rect(const glm::uvec2& pos, const glm::uvec2& size) const;

        void bind_render_resources(const BindGroup& bind_group);

        void set_camera_idx(uint32_t camera_idx) const;

        void set_material_idx(uint32_t idx) const;

        void draw(uint32_t num_indices, uint32_t first_index = 0, uint32_t num_instances = 1) const;

        /*!
         * \brief Preforms all the necessary tasks to prepare this command list for submission to the GPU, including ending any pending
         * render passes
         */
        void prepare_for_submission() override;

    protected:
        bool in_render_pass{false};

        const RenderPipelineState* current_render_pipeline_state{nullptr};

        bool is_render_material_bound{false};
    };
} // namespace rhi
