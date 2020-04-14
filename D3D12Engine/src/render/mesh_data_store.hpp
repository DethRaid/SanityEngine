#pragma once

#include "resources.hpp"

namespace render {
    /*!
     * \brief Binding for a vertex buffer
     */
    struct VertexBufferBinding {
        /*!
         * \brief The buffer to bind
         */
        Buffer* buffer;

        /*!
         * \brief Offset in bytes where the relevant data starts
         */
        size_t offset;

        /*!
         * \brief Size of a vertex, in bytes
         */
        size_t vertex_size;
    };

    class MeshDataStore {
    public:
        const rx::vector<VertexBufferBinding>& get_vertex_bindings() const;
        const Buffer& get_index_buffer() const;
    };
} // namespace render
