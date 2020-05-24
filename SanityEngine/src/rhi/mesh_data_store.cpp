#include "mesh_data_store.hpp"

#include <algorithm>

#include <spdlog/sinks/stdout_color_sinks.h>

#include "render_device.hpp"

namespace rhi {
    MeshDataStore::MeshDataStore(RenderDevice& device_in, std::unique_ptr<Buffer> vertex_buffer_in, std::unique_ptr<Buffer> index_buffer_in)
        : logger{spdlog::stdout_color_st("MeshDataStore")},
          device{&device_in},
          vertex_buffer{std::move(vertex_buffer_in)},
          index_buffer{std::move(index_buffer_in)} {
        vertex_bindings.reserve(4);
        vertex_bindings.push_back(VertexBufferBinding{vertex_buffer.get(), offsetof(StandardVertex, position), sizeof(StandardVertex)});
        vertex_bindings.push_back(VertexBufferBinding{vertex_buffer.get(), offsetof(StandardVertex, normal), sizeof(StandardVertex)});
        vertex_bindings.push_back(VertexBufferBinding{vertex_buffer.get(), offsetof(StandardVertex, color), sizeof(StandardVertex)});
        vertex_bindings.push_back(VertexBufferBinding{vertex_buffer.get(), offsetof(StandardVertex, texcoord), sizeof(StandardVertex)});
    }

    MeshDataStore::~MeshDataStore() {
        device->schedule_buffer_destruction(std::move(vertex_buffer));
        device->schedule_buffer_destruction(std::move(index_buffer));
    }

    const std::vector<VertexBufferBinding>& MeshDataStore::get_vertex_bindings() const { return vertex_bindings; }

    const Buffer& MeshDataStore::get_index_buffer() const { return *index_buffer; }

    std::pair<uint32_t, uint32_t> MeshDataStore::add_mesh(const std::vector<StandardVertex>& vertices,
                                                          const std::vector<uint32_t>& indices,
                                                          ResourceCommandList& commands) {
        logger->debug("Adding mesh with {} vertices and {} indices", vertices.size(), indices.size());
        logger->trace("Current vertex offset: {} Current index offset: {}", next_vertex_offset, next_index_offset);

        const auto vertex_data_size = static_cast<uint32_t>(vertices.size() * sizeof(StandardVertex));
        const auto index_data_size = static_cast<uint32_t>(indices.size() * sizeof(uint32_t));

        // Offset the indices so they'll refer to the right vertex
        std::vector<uint32_t> offset_indices;
        offset_indices.reserve(indices.size());

        std::transform(indices.begin(), indices.end(), std::back_inserter(offset_indices), [&](const uint32_t idx) {
            logger->trace("idx: {} new idx: {}", idx, idx + next_vertex_offset);
            return idx + next_vertex_offset;
        });

        logger->trace("Copying {} bytes of vertex data into the vertex buffer, offset of {}", vertex_data_size, next_free_vertex_byte);
        commands.copy_data_to_buffer(vertices.data(), vertex_data_size, *vertex_buffer, next_free_vertex_byte);

        const auto index_buffer_byte_offset = static_cast<uint32_t>(next_index_offset * sizeof(uint32_t));
        logger->trace("Copying {} bytes of index data into the index buffer, offset of {}", index_data_size, index_buffer_byte_offset);
        commands.copy_data_to_buffer(offset_indices.data(), index_data_size, *index_buffer, index_buffer_byte_offset);

        const auto vertex_offset = static_cast<uint32_t>(next_free_vertex_byte / sizeof(StandardVertex));

        next_free_vertex_byte += vertex_data_size;

        const auto index_offset = next_index_offset;

        next_vertex_offset += static_cast<uint32_t>(vertices.size());
        next_index_offset += static_cast<uint32_t>(indices.size());

        logger->trace("New vertex offset: {} New index offset: {}", next_vertex_offset, next_index_offset);

        return {vertex_offset, index_offset};
    }

    void MeshDataStore::bind_to_command_list(const ComPtr<ID3D12GraphicsCommandList4>& commands) const {
        const auto& vertex_bindings = get_vertex_bindings();

        // If we have more than 16 vertex attributes, we probably have bigger problems
        std::array<D3D12_VERTEX_BUFFER_VIEW, 16> vertex_buffer_views{};
        for(uint32_t i = 0; i < vertex_bindings.size(); i++) {
            const auto& binding = vertex_bindings[i];
            const auto* buffer = static_cast<const Buffer*>(binding.buffer);

            D3D12_VERTEX_BUFFER_VIEW view{};
            view.BufferLocation = buffer->resource->GetGPUVirtualAddress() + binding.offset;
            view.SizeInBytes = buffer->size - binding.offset;
            view.StrideInBytes = binding.vertex_size;

            vertex_buffer_views[i] = view;
        }

        commands->IASetVertexBuffers(0, static_cast<UINT>(vertex_bindings.size()), vertex_buffer_views.data());

        const auto& index_buffer = get_index_buffer();

        D3D12_INDEX_BUFFER_VIEW index_view{};
        index_view.BufferLocation = index_buffer.resource->GetGPUVirtualAddress();
        index_view.SizeInBytes = index_buffer.size;
        index_view.Format = DXGI_FORMAT_R32_UINT;

        commands->IASetIndexBuffer(&index_view);

        commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }
} // namespace rhi
