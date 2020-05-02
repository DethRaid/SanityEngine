#include "d3d12_resource_command_list.hpp"

#include <minitrace.h>

#include "d3dx12.hpp"

using std::move;

namespace rhi {
    D3D12ResourceCommandList::D3D12ResourceCommandList(ComPtr<ID3D12GraphicsCommandList> cmds, D3D12RenderDevice& device_in)
        : D3D12CommandList{move(cmds)}, device{&device_in} {}

    D3D12ResourceCommandList::D3D12ResourceCommandList(D3D12ResourceCommandList&& old) noexcept
        : D3D12CommandList(move(old)), device{old.device} {
        old.device = nullptr;
    }

    D3D12ResourceCommandList& D3D12ResourceCommandList::operator=(D3D12ResourceCommandList&& old) noexcept {
        device = old.device;

        old.device = nullptr;

        return static_cast<D3D12ResourceCommandList&>(D3D12CommandList::operator=(move(old)));
    }

    void D3D12ResourceCommandList::copy_data_to_buffer(const void* data,
                                                       const size_t num_bytes,
                                                       const Buffer& buffer,
                                                       const size_t offset) {
        MTR_SCOPE("D32D12ResourceCommandList", "copy_data_to_buffer");

        const auto& d3d12_buffer = static_cast<const D3D12Buffer&>(buffer);
        if(d3d12_buffer.mapped_ptr != nullptr) {
            // Copy the data directly, ezpz
            uint8_t* ptr = reinterpret_cast<uint8_t*>(d3d12_buffer.mapped_ptr);
            memcpy(ptr + offset, data, num_bytes);

        } else {
            // Upload the data using a staging buffer

            // TODO: Don't use a staging buffer on UMA, but that has a lot of sync implications and I'm scared

            auto staging_buffer = device->get_staging_buffer(num_bytes);
            memcpy(staging_buffer.ptr, data, num_bytes);

            set_resource_state(staging_buffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
            set_resource_state(d3d12_buffer, D3D12_RESOURCE_STATE_COPY_DEST);

            commands->CopyBufferRegion(d3d12_buffer.resource.Get(), offset, staging_buffer.resource.Get(), 0, num_bytes);

            add_completion_function(
                [staging_buffer{move(staging_buffer)}, this]() mutable { device->return_staging_buffer(move(staging_buffer)); });

            command_types.insert(D3D12_COMMAND_LIST_TYPE_COPY);
        }
    }

    void D3D12ResourceCommandList::copy_data_to_image(const void* data, const Image& image) {
        MTR_SCOPE("D3D12ResourceCommandList", "copy_data_to_image");

        const auto bytes_per_pixel = size_in_bytes(image.format);
        const auto num_bytes = image.width * image.height * image.depth * bytes_per_pixel;

        // TODO: Don't use a staging buffer on UMA

        auto staging_buffer = device->get_staging_buffer(num_bytes);
        memcpy(staging_buffer.ptr, data, num_bytes);

        const auto& d3d12_image = static_cast<const D3D12Image&>(image);

        set_resource_state(staging_buffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
        set_resource_state(d3d12_image, D3D12_RESOURCE_STATE_COPY_DEST);

        D3D12_SUBRESOURCE_DATA subresource;
        subresource.pData = data;
        subresource.RowPitch = d3d12_image.width * bytes_per_pixel;
        subresource.SlicePitch = d3d12_image.width * d3d12_image.height * bytes_per_pixel;

        UpdateSubresources(commands.Get(), d3d12_image.resource.Get(), staging_buffer.resource.Get(), 0, 0, 1, &subresource);

        device->return_staging_buffer(move(staging_buffer));

        command_types.insert(D3D12_COMMAND_LIST_TYPE_COPY);
    }
} // namespace rhi
