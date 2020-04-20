#pragma once
#include "../render_command_list.hpp"
#include "d3d12_compute_command_list.hpp"
#include "d3d12_render_pipeline_state.hpp"

namespace rhi {
    class D3D12RenderCommandList final : public D3D12ComputeCommandList, public virtual RenderCommandList {
    public:
        D3D12RenderCommandList(ComPtr<ID3D12GraphicsCommandList> cmds, D3D12RenderDevice& device_in);

        D3D12RenderCommandList(const D3D12RenderCommandList& other) = delete;
        D3D12RenderCommandList& operator=(const D3D12RenderCommandList& other) = delete;

        D3D12RenderCommandList(D3D12RenderCommandList&& old) noexcept;
        D3D12RenderCommandList& operator=(D3D12RenderCommandList&& old) noexcept;

        using D3D12ComputeCommandList::set_pipeline_state;

#pragma region RenderCommandList
        ~D3D12RenderCommandList() override = default;

        void set_framebuffer(const Framebuffer& framebuffer) override;

        void set_pipeline_state(const RenderPipelineState& state) override;

        void bind_render_resources(const BindGroup& resources) override;

        void bind_mesh_data(const MeshDataStore& mesh_data) override;

        void draw(uint32_t num_indices, uint32_t first_index, uint32_t num_instances) override;
#pragma endregion

        /*!
         * \brief Preforms all the necessary tasks to prepare this command list for submission to the GPU, including ending any pending render passes
         */
        void prepare_for_submission() override;

    protected:
        ComPtr<ID3D12GraphicsCommandList4> commands4;

        bool in_render_pass{false};

        const D3D12RenderPipelineState* current_render_pipeline_state{nullptr};

        bool is_render_material_bound{false};
        bool is_mesh_data_bound{false};
    };
} // namespace render
