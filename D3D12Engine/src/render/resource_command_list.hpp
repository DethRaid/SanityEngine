#pragma once

#include "command_list.hpp"

namespace render {
    /*!
     * \brief A command list which can record operations on resources
     *
     * This includes your basic CRUD operations, mostly
     */
    class ResourceCommandList : public virtual CommandList {
    public:
        virtual void create_image() = 0;

        virtual void destroy_image() = 0;

        virtual void create_buffer() = 0;

        virtual void copy_data_to_buffer() = 0;

        virtual void copy_buffer_to_buffer() = 0;

        virtual void delete_buffer() = 0;
    };
} // namespace render
