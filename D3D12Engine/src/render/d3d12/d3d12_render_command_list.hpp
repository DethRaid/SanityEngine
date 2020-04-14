#pragma once
#include "../render_command_list.hpp"
#include "d3d12_compute_command_list.hpp"

namespace render {
    class D3D12RenderCommandList : public D3D12ComputeCommandList, public virtual RenderCommandList {
    public:
        D3D12RenderCommandList(rx::memory::allocator& allocator, ComPtr<ID3D12GraphicsCommandList> cmds, D3D12RenderDevice& device_in);

#pragma region RenderCommandList
        ~D3D12RenderCommandList() override = default;

        void set_render_targets(const rx::vector<Image*>& color_targets, Image* depth_target) override;

        void set_camera_buffer(const Buffer& camera_buffer) override;

        void set_material_data_buffer(const Buffer& material_data_buffer) override;

        void set_textures_array(const rx::vector<Image*>& textures) override;

        void bind_mesh_data(const MeshDataStore& mesh_data) override;

        void draw(uint32_t num_indices, uint32_t first_index, uint32_t num_instances) override;

        void present_backbuffer() override;
#pragma endregion

    protected:
        ComPtr<ID3D12GraphicsCommandList4> commands4;
    };
} // namespace render
