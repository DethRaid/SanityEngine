#pragma once
#include "rx/core/string.h"

namespace render {
    struct Buffer {
        size_t size{};
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
        rx::string name{};

        BufferUsage usage;
        size_t size{0};
    };

    enum class ImageUsage {
        RenderTarget,
        DepthStencil,
        SampledImage,
        UnorderedAccess
    };

    enum class ImageFormat {
        Rgba8,
        Rgba32F,
        Depth32,
        Depth24Stencil8,
    };

    struct Image {
    };

    struct ImageCreateInfo {
        rx::string name;

        ImageUsage usage;
        ImageFormat format;

        size_t width{0};
        size_t height{0};
        size_t depth{0};
    };
} // namespace render
