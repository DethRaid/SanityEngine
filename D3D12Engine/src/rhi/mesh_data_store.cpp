#include "mesh_data_store.hpp"

namespace render {
    MeshDataStore::MeshDataStore(std::unique_ptr<Buffer> vertex_buffer_in, std::unique_ptr<Buffer> index_buffer_in)
        : vertex_buffer{std::move(vertex_buffer_in)}, index_buffer{std::move(index_buffer_in)} {
        vertex_bindings.reserve(5);
        vertex_bindings.push_back(VertexBufferBinding{vertex_buffer.get(), offsetof(BveVertex, position), sizeof(BveVertex)});
        vertex_bindings.push_back(VertexBufferBinding{vertex_buffer.get(), offsetof(BveVertex, normal), sizeof(BveVertex)});
        vertex_bindings.push_back(VertexBufferBinding{vertex_buffer.get(), offsetof(BveVertex, color), sizeof(BveVertex)});
        vertex_bindings.push_back(VertexBufferBinding{vertex_buffer.get(), offsetof(BveVertex, texcoord), sizeof(BveVertex)});
        vertex_bindings.push_back(VertexBufferBinding{vertex_buffer.get(), offsetof(BveVertex, double_sided), sizeof(BveVertex)});
    }

    const std::vector<VertexBufferBinding>& MeshDataStore::get_vertex_bindings() const { return vertex_bindings; }

    const Buffer& MeshDataStore::get_index_buffer() const { return *index_buffer; }
} // namespace render
