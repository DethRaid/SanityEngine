#include "mesh_data_store.hpp"

#include <algorithm>

namespace rhi {
    MeshDataStore::MeshDataStore(RenderDevice& device_in, std::unique_ptr<Buffer> vertex_buffer_in, std::unique_ptr<Buffer> index_buffer_in)
        : device{&device_in}, vertex_buffer{std::move(vertex_buffer_in)}, index_buffer{std::move(index_buffer_in)} {
        vertex_bindings.reserve(4);
        vertex_bindings.push_back(VertexBufferBinding{vertex_buffer.get(), offsetof(BveVertex, position), sizeof(BveVertex)});
        vertex_bindings.push_back(VertexBufferBinding{vertex_buffer.get(), offsetof(BveVertex, normal), sizeof(BveVertex)});
        vertex_bindings.push_back(VertexBufferBinding{vertex_buffer.get(), offsetof(BveVertex, color), sizeof(BveVertex)});
        vertex_bindings.push_back(VertexBufferBinding{vertex_buffer.get(), offsetof(BveVertex, texcoord), sizeof(BveVertex)});
    }

    MeshDataStore::~MeshDataStore() {
        device->destroy_buffer(std::move(vertex_buffer));
        device->destroy_buffer(std::move(index_buffer));
    }

    const std::vector<VertexBufferBinding>& MeshDataStore::get_vertex_bindings() const { return vertex_bindings; }

    const Buffer& MeshDataStore::get_index_buffer() const { return *index_buffer; }

    uint32_t MeshDataStore::add_mesh(const std::vector<BveVertex>& vertices, const std::vector<uint32_t>& indices) {
        const auto vertex_data_size = vertices.size() * sizeof(BveVertex);
        const auto index_data_size = indices.size() * sizeof(uint32_t);

        // Offset the indices so they'll refer to the right vertex
        std::vector<uint32_t> offset_indices;
        offset_indices.reserve(indices.size());

        std::transform(indices.begin(), indices.end(), std::back_inserter(offset_indices), [&](const uint32_t idx) {
            return idx + next_free_index;
        });

        auto cmds = device->create_resource_command_list();
        cmds->copy_data_to_buffer(vertices.data(), vertex_data_size, *vertex_buffer, next_free_vertex_byte);

        cmds->copy_data_to_buffer(offset_indices.data(), index_data_size, *index_buffer, next_free_index * sizeof(uint32_t));

        device->submit_command_list(std::move(cmds));

        next_free_vertex_byte += vertex_data_size;

        const auto mesh_start_idx = next_free_index;
        next_free_index += indices.size();

        return mesh_start_idx;
    }
} // namespace rhi
