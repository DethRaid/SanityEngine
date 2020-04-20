#pragma once

#include "../resource_command_list.hpp"
#include "d3d12_command_list.hpp"
#include "d3d12_render_device.hpp"

namespace rhi {
    class D3D12ResourceCommandList : public D3D12CommandList,  public virtual ResourceCommandList {
    public:
        D3D12ResourceCommandList(ComPtr<ID3D12GraphicsCommandList> cmds, D3D12RenderDevice& device_in);

        D3D12ResourceCommandList(const D3D12ResourceCommandList& other) = delete;
        D3D12ResourceCommandList& operator=(const D3D12ResourceCommandList& other) = delete;

        D3D12ResourceCommandList(D3D12ResourceCommandList&& old) noexcept;
        D3D12ResourceCommandList& operator=(D3D12ResourceCommandList&& old) noexcept;

#pragma region ResourceCommandList
        ~D3D12ResourceCommandList() override = default;

        void copy_data_to_buffer(void* data, size_t num_bytes, const Buffer& buffer, size_t offset) override;

        void copy_data_to_image(void* data, const Image& image) override;
#pragma endregion

    protected:
        D3D12RenderDevice* device;
    };
} // namespace render
