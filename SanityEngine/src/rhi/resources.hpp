#pragma once

#include <string>

namespace rhi {
    struct Buffer {
        std::string name;

        size_t size{};

        virtual ~Buffer() = default;
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
    };

    struct BufferCreateInfo {
        std::string name{};

        BufferUsage usage;
        size_t size{0};
    };

    enum class ImageUsage { RenderTarget, DepthStencil, SampledImage, UnorderedAccess };

    enum class ImageFormat {
        Rgba8,
        Rgba32F,
        Depth32,
        Depth24Stencil8,
    };

    struct Image {
        std::string name;

        size_t width{1};
        size_t height{1};
        size_t depth{1};

        ImageFormat format{};

        virtual ~Image() = default;
    };

    struct ImageCreateInfo {
        std::string name;

        ImageUsage usage;
        ImageFormat format;

        size_t width{1};
        size_t height{1};
        size_t depth{1};
    };

    [[nodiscard]] size_t size_in_bytes(ImageFormat format);
} // namespace render
