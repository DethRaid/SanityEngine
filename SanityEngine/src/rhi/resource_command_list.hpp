#pragma once

#include "command_list.hpp"
#include "resources.hpp"

namespace rhi {
    struct BufferCreateInfo;

    /*!
     * \brief A command list which can record operations on resources
     *
     * This includes your basic CRUD operations, mostly
     */
    class ResourceCommandList : public virtual CommandList {
    public:
        virtual ~ResourceCommandList() = default;

        /*!
         * \brief Copies data to a buffer
         *
         * This method should be used for large, one-time data transfers. Uploading mesh data is the perfect example - you generally have a
         * large-ish chunk of data to upload, and you only upload it one time
         *
         * This method should _not_ be used for things like updating individual model matrices. For those kinds of data transfers, you
         * should map the buffer the write to the mapped pointer
         *
         * \param data The data to copy to the buffer
         * \param num_bytes The number of bytes to copy to the buffer
         * \param buffer The buffer that will receive the data
         * \param offset The offset into the buffer to copy data to
         */
        virtual void copy_data_to_buffer(const void* data, size_t num_bytes, const Buffer& buffer, size_t offset) = 0;

        /*!
         * \brief Copies data to an image
         *
         * This method will copy enough data to completely fill the image. Thus, the data pointed to by `data` must be at least as large as
         *the image
         *
         * \param data The data to copy
         * \param image The image to copy data to
         */
        virtual void copy_data_to_image(const void* data, const Image& image) = 0;
    };
} // namespace render
