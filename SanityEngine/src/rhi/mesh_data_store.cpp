#include "mesh_data_store.hpp"

#include <Tracy.hpp>
#include <TracyD3D12.hpp>
#include <d3d11_1.h>
#include <rx/core/log.h>

#include "rhi/helpers.hpp"
#include "rhi/render_device.hpp"

namespace renderer {
    RX_LOG("MeshDataStore", logger);

    MeshDataStore::MeshDataStore(RenderDevice& device_in, Rx::Ptr<Buffer> vertex_buffer_in, Rx::Ptr<Buffer> index_buffer_in)
        : device{&device_in}, vertex_buffer{std::move(vertex_buffer_in)}, index_buffer{std::move(index_buffer_in)} {

        vertex_bindings.reserve(4);
        vertex_bindings.push_back(VertexBufferBinding{vertex_buffer.get(), offsetof(StandardVertex, position), sizeof(StandardVertex)});
        vertex_bindings.push_back(VertexBufferBinding{vertex_buffer.get(), offsetof(StandardVertex, normal), sizeof(StandardVertex)});
        vertex_bindings.push_back(VertexBufferBinding{vertex_buffer.get(), offsetof(StandardVertex, color), sizeof(StandardVertex)});
        vertex_bindings.push_back(VertexBufferBinding{vertex_buffer.get(), offsetof(StandardVertex, texcoord), sizeof(StandardVertex)});
    }

    const Rx::Vector<VertexBufferBinding>& MeshDataStore::get_vertex_bindings() const { return vertex_bindings; }

    const Buffer& MeshDataStore::get_index_buffer() const { return *index_buffer; }

    Mesh MeshDataStore::add_mesh(const Rx::Vector<StandardVertex>& vertices,
                                 const Rx::Vector<Uint32>& indices,
                                 ID3D11DeviceContext* context) {
        ZoneScoped;

        SCOPED_EVENT(context, "MeshDataStore::add_mesh");

        logger->verbose("Adding mesh with %u vertices and %u indices", vertices.size(), indices.size());

        const auto vertex_data_size = static_cast<Uint32>(vertices.size() * sizeof(StandardVertex));
        const auto index_data_size = static_cast<Uint32>(indices.size() * sizeof(Uint32));

        // Offset the indices so they'll refer to the right vertex
        Rx::Vector<Uint32> offset_indices;
        offset_indices.reserve(indices.size());

        indices.each_fwd([&](const Uint32 idx) { offset_indices.push_back(idx + next_vertex_offset); });

        auto* vertex_resource = vertex_buffer->resource.Get();
        auto* index_resource = index_buffer->resource.Get();

        const auto index_buffer_byte_offset = static_cast<Uint32>(next_index_offset * sizeof(Uint32));

        upload_data_with_staging_buffer(context, *device, vertex_resource, vertices.data(), vertex_data_size, next_free_vertex_byte);

        upload_data_with_staging_buffer(context, *device, index_resource, offset_indices.data(), index_data_size, index_buffer_byte_offset);

        const auto vertex_offset = static_cast<Uint32>(next_free_vertex_byte / sizeof(StandardVertex));

        next_free_vertex_byte += vertex_data_size;

        const auto index_offset = next_index_offset;

        next_vertex_offset += static_cast<Uint32>(vertices.size());
        next_index_offset += static_cast<Uint32>(indices.size());

        return {.first_vertex = vertex_offset,
                .num_vertices = static_cast<Uint32>(vertices.size()),
                .first_index = index_offset,
                .num_indices = static_cast<Uint32>(indices.size())};
    }

    void MeshDataStore::bind_to_context(ID3D11DeviceContext* context) const {
        const auto& vertex_bindings = get_vertex_bindings();

        // If we have more than 16 vertex attributes, we probably have bigger problems
        Rx::Array<ID3D11Buffer* [16]> vertex_buffers {};
        Rx::Array<UINT[16]> vertex_strides;
        Rx::Array<UINT[16]> vertex_offsets;
        for(Uint32 i = 0; i < vertex_bindings.size(); i++) {
            const auto& binding = vertex_bindings[i];
            const auto* buffer = static_cast<const Buffer*>(binding.buffer);

            vertex_buffers[i] = binding.buffer->resource.Get();
            vertex_strides[i] = binding.vertex_size;
            vertex_offsets[i] = binding.offset;
        }

        context->IASetVertexBuffers(0,
                                    static_cast<UINT>(vertex_bindings.size()),
                                    vertex_buffers.data(),
                                    vertex_strides.data(),
                                    vertex_offsets.data());

        const auto& index_buffer = get_index_buffer();

        context->IASetIndexBuffer(index_buffer.resource.Get(), DXGI_FORMAT_R32_UINT, 0);

        context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }
} // namespace renderer
