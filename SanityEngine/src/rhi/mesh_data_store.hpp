#pragma once

#include <memory>
#include <vector>

#include <DirectXMath.h>

#include "render_device.hpp"
#include "resources.hpp"

struct BveVertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    uint32_t color;
    DirectX::XMFLOAT2 texcoord;
    uint32_t double_sided;
};

namespace rhi {
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
        MeshDataStore(RenderDevice& device_in, std::unique_ptr<Buffer> vertex_buffer_in, std::unique_ptr<Buffer> index_buffer_in);

        MeshDataStore(const MeshDataStore& other) = delete;
        MeshDataStore& operator=(const MeshDataStore& other) = delete;

        MeshDataStore(MeshDataStore&& old) noexcept = default;
        MeshDataStore& operator=(MeshDataStore&& old) noexcept = delete;

        ~MeshDataStore();

        [[nodiscard]] const std::vector<VertexBufferBinding>& get_vertex_bindings() const;

        [[nodiscard]] const Buffer& get_index_buffer() const;

        [[nodiscard]] uint32_t add_mesh(const std::vector<BveVertex>& vertices, const std::vector<uint32_t>& indices);

    private:
        RenderDevice* device;

        std::unique_ptr<Buffer> vertex_buffer;

        std::unique_ptr<Buffer> index_buffer;

        std::vector<VertexBufferBinding> vertex_bindings{};

        /*!
         * \brief Index of the byte in the vertex buffer where the next mesh can be uploaded to
         *
         * I'll eventually need a way to unload meshes, but that's more complicated
         */
        uint32_t next_free_vertex_byte{0};

        /*!
         * \brief The next mesh index
         */
        uint32_t next_free_index{0};
    };
} // namespace rhi
