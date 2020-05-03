#include "mesh_data_store.hpp"

#include <algorithm>

#include <spdlog/sinks/stdout_color_sinks.h>

namespace rhi {
    MeshDataStore::MeshDataStore(RenderDevice& device_in, std::unique_ptr<Buffer> vertex_buffer_in, std::unique_ptr<Buffer> index_buffer_in)
        : logger{spdlog::stdout_color_st("MeshDataStore")},
          device{&device_in},
          vertex_buffer{std::move(vertex_buffer_in)},
          index_buffer{std::move(index_buffer_in)} {
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
        logger->debug("Adding mesh with {} vertices and {} indices", vertices.size(), indices.size());
        logger->trace("Current vertex offset: {} Current index offset: {}",
                      next_vertex_offset,
                      next_index_offset);

        const auto vertex_data_size = static_cast<uint32_t>(vertices.size() * sizeof(BveVertex));
        const auto index_data_size = static_cast<uint32_t>(indices.size() * sizeof(uint32_t));

        // Offset the indices so they'll refer to the right vertex
        std::vector<uint32_t> offset_indices;
        offset_indices.reserve(indices.size());

        std::transform(indices.begin(), indices.end(), std::back_inserter(offset_indices), [&](const uint32_t idx) {
            logger->trace("idx: {} new idx: {}", idx, idx + next_vertex_offset);
            return idx + next_vertex_offset;
        });

        auto cmds = device->create_resource_command_list();

        logger->trace("Copying {} bytes of vertex data into the vertex buffer, offset of {}", vertex_data_size, next_free_vertex_byte);
        cmds->copy_data_to_buffer(vertices.data(), vertex_data_size, *vertex_buffer, next_free_vertex_byte);

        const auto index_buffer_byte_offset = static_cast<uint32_t>(next_index_offset * sizeof(uint32_t));
        logger->trace("Copying {} bytes of index data into the index buffer, offset of {}", index_data_size, index_buffer_byte_offset);
        cmds->copy_data_to_buffer(offset_indices.data(), index_data_size, *index_buffer, index_buffer_byte_offset);

        device->submit_command_list(std::move(cmds));

        next_free_vertex_byte += vertex_data_size;

        const auto mesh_start_idx = next_index_offset;

        next_vertex_offset += static_cast<uint32_t>(vertices.size());
        next_index_offset += static_cast<uint32_t>(indices.size());

        logger->trace("New vertex offset: {} New index offset: {}", next_vertex_offset, next_index_offset);

        return mesh_start_idx;
    }
} // namespace rhi
