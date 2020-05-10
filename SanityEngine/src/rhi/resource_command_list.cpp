#include "resource_command_list.hpp"

#include <minitrace.h>
#include <spdlog/spdlog.h>

#include "../core/defer.hpp"
#include "d3dx12.hpp"
#include "helpers.hpp"
#include "render_device.hpp"

using std::move;

namespace rhi {
    ResourceCommandList::ResourceCommandList(ComPtr<ID3D12GraphicsCommandList4> cmds, RenderDevice& device_in)
        : CommandList{move(cmds)}, device{&device_in} {}

    ResourceCommandList::ResourceCommandList(ResourceCommandList&& old) noexcept : CommandList(move(old)), device{old.device} {
        old.device = nullptr;
    }

    ResourceCommandList& ResourceCommandList::operator=(ResourceCommandList&& old) noexcept {
        device = old.device;

        old.device = nullptr;

        return static_cast<ResourceCommandList&>(CommandList::operator=(move(old)));
    }

    void ResourceCommandList::copy_data_to_buffer(const void* data, const uint32_t num_bytes, const Buffer& buffer, const uint32_t offset) {
        if(buffer.mapped_ptr != nullptr) {
            MTR_SCOPE("D32D12ResourceCommandList", "copy_data_to_buffer");
            // Copy the data directly, ezpz
            uint8_t* ptr = static_cast<uint8_t*>(buffer.mapped_ptr);
            memcpy(ptr + offset, data, num_bytes);

        } else {
            // Upload the data using a staging buffer

            auto staging_buffer = device->get_staging_buffer(num_bytes);
            memcpy(staging_buffer.ptr, data, num_bytes);

            set_resource_state(staging_buffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
            set_resource_state(buffer, D3D12_RESOURCE_STATE_COPY_DEST);

            commands->CopyBufferRegion(buffer.resource.Get(), offset, staging_buffer.resource.Get(), 0, num_bytes);

            DEFER(a, [&]() { device->return_staging_buffer(move(staging_buffer)); });

            command_types.insert(D3D12_COMMAND_LIST_TYPE_COPY);
        }
    }

    void ResourceCommandList::copy_data_to_image(const void* data, const Image& image) {
        const auto bytes_per_pixel = size_in_bytes(image.format);
        const auto num_bytes = image.width * image.height * image.depth * bytes_per_pixel;

        // TODO: Don't use a staging buffer on UMA

        auto staging_buffer = device->get_staging_buffer(num_bytes * 2);
        memcpy(staging_buffer.ptr, data, num_bytes);

        set_resource_state(staging_buffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
        set_resource_state(image, D3D12_RESOURCE_STATE_COPY_DEST);

        D3D12_SUBRESOURCE_DATA subresource;
        subresource.pData = data;
        subresource.RowPitch = image.width * bytes_per_pixel;
        subresource.SlicePitch = image.width * image.height * bytes_per_pixel;

        const auto result = UpdateSubresources(commands.Get(), image.resource.Get(), staging_buffer.resource.Get(), 0, 0, 1, &subresource);
        if(result == 0 || FAILED(result)) {
            spdlog::error("Could not copy data to image");
        }

        DEFER(a, [&]() { device->return_staging_buffer(move(staging_buffer)); });

        command_types.insert(D3D12_COMMAND_LIST_TYPE_COPY);
    }
} // namespace rhi
