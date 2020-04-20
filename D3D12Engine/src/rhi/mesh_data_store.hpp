#pragma once

#include <memory>
#include <vector>

#include <DirectXMath.h>

#include "resources.hpp"

namespace rhi {
    struct BveVertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
        DirectX::XMVECTORU8 color;
        DirectX::XMFLOAT2 texcoord;
        uint32_t double_sided;
    };

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
        MeshDataStore(std::unique_ptr<Buffer> vertex_buffer_in, std::unique_ptr<Buffer> index_buffer_in);

        MeshDataStore(const MeshDataStore& other) = delete;
        MeshDataStore& operator=(const MeshDataStore& other) = delete;

        MeshDataStore(MeshDataStore&& old) noexcept = default;
        MeshDataStore& operator=(MeshDataStore&& old) noexcept = delete;

        ~MeshDataStore() = default;

        [[nodiscard]] const std::vector<VertexBufferBinding>& get_vertex_bindings() const;

        [[nodiscard]] const Buffer& get_index_buffer() const;

    private:
        std::unique_ptr<Buffer> vertex_buffer;

        std::unique_ptr<Buffer> index_buffer;

        std::vector<VertexBufferBinding> vertex_bindings{};
    };
} // namespace render
