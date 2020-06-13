#pragma once

#include <cstdint>
#include <concepts>

#include <d3d12.h>
#include <rx/core/string.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace D3D12MA {
    class Allocation;
}

namespace rhi {
    struct Buffer {
        Rx::String name;

        uint32_t size{};

        ComPtr<ID3D12Resource> resource;

        D3D12MA::Allocation* allocation;

        void* mapped_ptr{nullptr};
    };

    struct StagingBuffer : Buffer {
        void* ptr{nullptr};
    };

    /*!
     * \brief All the possible ways that one can use a buffer
     */
    enum class BufferUsage {
        StagingBuffer,
        IndexBuffer,
        VertexBuffer,
        ConstantBuffer,
        IndirectCommands,
        UnorderedAccess,
        RaytracingAccelerationStructure,

        /*!
         * \brief Vertex buffer that gets written to every frame
         */
        UiVertices,
    };

    struct BufferCreateInfo {
        Rx::String name{};

        BufferUsage usage;
        uint32_t size{0};
    };

    enum class ImageUsage { RenderTarget, DepthStencil, SampledImage, UnorderedAccess };

    enum class ImageFormat {
        Rgba8,
        R32F,
        Rgba32F,
        Depth32,
        Depth24Stencil8,
    };

    struct Image {
        Rx::String name;

        uint32_t width{1};
        uint32_t height{1};
        uint32_t depth{1};

        ComPtr<ID3D12Resource> resource;

        D3D12MA::Allocation* allocation;

        ImageFormat format;
    };

    struct ImageCreateInfo {
        Rx::String name;

        ImageUsage usage;
        ImageFormat format;

        uint32_t width{1};
        uint32_t height{1};
        uint32_t depth{1};

        /*!
         * \brief If true, this resource may be shared with other APIs, such as CUDA
         */
        bool enable_resource_sharing{false};
    };

    [[nodiscard]] uint32_t size_in_bytes(ImageFormat format);

    template <typename T>
    concept GpuResource = requires(T a) {
        { a.allocation }
        ->std::convertible_to<D3D12MA::Allocation*>;
    };
} // namespace rhi
