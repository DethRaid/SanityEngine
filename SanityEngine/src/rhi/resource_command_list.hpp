#pragma once

#include "command_list.hpp"
#include "resources.hpp"

namespace rhi {
    class RenderDevice;
    struct BufferCreateInfo;

    /*!
     * \brief A command list which can record operations on resources
     *
     * This includes your basic CRUD operations, mostly
     */
    class ResourceCommandList : public CommandList {
    public:
        ResourceCommandList(ComPtr<ID3D12GraphicsCommandList4> cmds, RenderDevice& device_in, ID3D12InfoQueue* info_queue_in);

        ResourceCommandList(const ResourceCommandList& other) = delete;
        ResourceCommandList& operator=(const ResourceCommandList& other) = delete;

        ResourceCommandList(ResourceCommandList&& old) noexcept;
        ResourceCommandList& operator=(ResourceCommandList&& old) noexcept;

        void copy_data_to_buffer(const void* data, uint32_t num_bytes, const Buffer& buffer, uint32_t offset = 0);

        void copy_data_to_image(const void* data, const Image& image);

        /*!
         * \brief Copies the contents of a render targets into an image
         *
         * The image and render target must have the same size and pixel format. Mip 0 of the render target is copied into mip 0 of the
         * image, no mipmaps are automatically generated
         */
        void copy_render_target_to_image(const Image& source, const Image& destination);

    protected:
        RenderDevice* device;
    };
} // namespace rhi
