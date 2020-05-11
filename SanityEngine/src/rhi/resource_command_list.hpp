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
        ResourceCommandList(ComPtr<ID3D12GraphicsCommandList4> cmds, RenderDevice& device_in);

        ResourceCommandList(const ResourceCommandList& other) = delete;
        ResourceCommandList& operator=(const ResourceCommandList& other) = delete;

        ResourceCommandList(ResourceCommandList&& old) noexcept;
        ResourceCommandList& operator=(ResourceCommandList&& old) noexcept;

        void copy_data_to_buffer(const void* data, uint32_t num_bytes, const Buffer& buffer, uint32_t offset = 0);

        void copy_data_to_image(const void* data, const Image& image);

    protected:
        RenderDevice* device;
    };
} // namespace render
