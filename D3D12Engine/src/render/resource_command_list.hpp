#pragma once

#include <rx/core/function.h>

#include "command_list.hpp"
#include "resources.hpp"

namespace render {
    struct BufferCreateInfo;

    /*!
     * \brief A command list which can record operations on resources
     *
     * This includes your basic CRUD operations, mostly
     */
    class ResourceCommandList : public virtual CommandList {
    public:
        virtual void copy_data_to_buffer(void* data, const Buffer& buffer) = 0;

        virtual void dopy_data_to_image(void* data, const Image& image) = 0;
    };
} // namespace render
