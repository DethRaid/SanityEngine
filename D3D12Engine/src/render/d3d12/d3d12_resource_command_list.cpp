#include "d3d12_resource_command_list.hpp"

#include <minitrace.h>
#include <rx/core/utility/move.h>

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
        if(device->has_separate_device_memory()) {
            // Upload the data using a staging buffer
            auto staging_buffer = device->get_staging_buffer(num_bytes);
            memcpy(staging_buffer->ptr, data, num_bytes);

            const auto& d3d12_buffer = static_cast<const D3D12Buffer&>(buffer);

            commands->CopyBufferRegion(d3d12_buffer.resource.Get(), offset, staging_buffer->resource.Get(), 0, num_bytes);

            add_completion_function(
                [staging_buffer{move(staging_buffer)}, this]() mutable { device->return_staging_buffer(move(staging_buffer)); });

        } else {
        }
    }
} // namespace render
