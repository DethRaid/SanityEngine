#include "d3d12_resource_command_list.hpp"

#include <minitrace.h>
#include <rx/core/utility/move.h>

#include "d3dx12.hpp"
using rx::utility::move;

namespace render {
    D3D12ResourceCommandList::D3D12ResourceCommandList(rx::memory::allocator& allocator,
                                                       ComPtr<ID3D12GraphicsCommandList> cmds,
                                                       D3D12RenderDevice& device_in)
        : D3D12CommandList(allocator, move(cmds)), device{&device_in} {}

    D3D12ResourceCommandList::D3D12ResourceCommandList(D3D12ResourceCommandList&& old) noexcept
        : D3D12CommandList(move(old)), device{old.device} {
        old.device = nullptr;
    }

    D3D12ResourceCommandList& D3D12ResourceCommandList::operator=(D3D12ResourceCommandList&& old) noexcept {
        device = old.device;

        old.device = nullptr;

        return static_cast<D3D12ResourceCommandList&>(D3D12CommandList::operator=(move(old)));
    }

    void D3D12ResourceCommandList::copy_data_to_buffer(void* data, const size_t num_bytes, const Buffer& buffer, const size_t offset) {
        MTR_SCOPE("D32D12ResourceCommandList", "copy_data_to_buffer");

        // Upload the data using a staging buffer

        // TODO: Don't use a staging buffer on UMA, but that has a lot of sync implications and I'm scared

        auto staging_buffer = device->get_staging_buffer(num_bytes);
        memcpy(staging_buffer->ptr, data, num_bytes);

        const auto& d3d12_buffer = static_cast<const D3D12Buffer&>(buffer);

        if(auto* previous_state = most_recent_resource_states.find(d3d12_buffer.resource.Get())) {
            const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12_buffer.resource.Get(),
                                                                      *previous_state,
                                                                      D3D12_RESOURCE_STATE_COPY_DEST);
            commands->ResourceBarrier(1, &barrier);

            *previous_state = D3D12_RESOURCE_STATE_COPY_DEST;
        } else {
            most_recent_resource_states.insert(d3d12_buffer.resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
        }

        commands->CopyBufferRegion(d3d12_buffer.resource.Get(), offset, staging_buffer->resource.Get(), 0, num_bytes);

        add_completion_function(
            [staging_buffer{move(staging_buffer)}, this]() mutable { device->return_staging_buffer(move(staging_buffer)); });

        command_types.insert(D3D12_COMMAND_LIST_TYPE_COPY);
    }

    void D3D12ResourceCommandList::copy_data_to_image(void* data, const Image& image) {
        MTR_SCOPE("D3D12ResourceCommandList", "copy_data_to_image");

        const auto bytes_per_pixel = size_in_bytes(image.format);
        const auto num_bytes = image.width * image.height * image.depth * bytes_per_pixel;

        // TODO: Don't use a staging buffer on UMA

        auto staging_buffer = device->get_staging_buffer(num_bytes);
        memcpy(staging_buffer->ptr, data, num_bytes);

        const auto& d3d12_image = static_cast<const D3D12Image&>(image);

        D3D12_SUBRESOURCE_DATA subresource;
        subresource.pData = data;
        subresource.RowPitch = d3d12_image.width * bytes_per_pixel;
        subresource.SlicePitch = d3d12_image.width * d3d12_image.height * bytes_per_pixel;

        UpdateSubresources(commands.Get(), d3d12_image.resource.Get(), staging_buffer->resource.Get(), 0, 0, 1, &subresource);

        add_completion_function(
            [staging_buffer{move(staging_buffer)}, this]() mutable { device->return_staging_buffer(move(staging_buffer)); });

        command_types.insert(D3D12_COMMAND_LIST_TYPE_COPY);
    }
} // namespace render