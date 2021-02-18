#pragma once

#include <concepts>

#include <d3d12.h>

#include "core/types.hpp"
#include "renderer/handles.hpp"
#include "rx/core/string.h"

namespace D3D12MA {
    class Allocation;
}

namespace sanity::engine::renderer {
    struct Buffer {
        Rx::String name;

        Uint64 size{0};

        Uint64 alignment{0};

        ComPtr<ID3D12Resource> resource;

        D3D12MA::Allocation* allocation{nullptr};

        void* mapped_ptr{nullptr};
    };

    using BufferHandle = GpuResourceHandle<Buffer>;

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
        Uint32 size{0};
    };

    enum class TextureUsage { RenderTarget, DepthStencil, SampledTexture, UnorderedAccess };

    enum class TextureFormat {
        Rgba8,
        R32F,
        R32UInt,
        Rg16F,
        Rgba32F,
        Depth32,
        Depth24Stencil8,
        Rg32F,
    };

    struct Texture {
        Rx::String name;

        Uint32 width{1};
        Uint32 height{1};
        Uint32 depth{1};

        ComPtr<ID3D12Resource> resource;

        D3D12MA::Allocation* allocation;

        TextureFormat format;
    };

	using TextureHandle = GpuResourceHandle<Texture>;

    struct RenderTarget : Texture {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
    };

    struct TextureCreateInfo {
        Rx::String name;

        TextureUsage usage;
        TextureFormat format;

        Uint32 width{1};
        Uint32 height{1};
        Uint32 depth{1};

        /*!
         * \brief If true, this resource may be shared with other APIs, such as CUDA
         */
        bool enable_resource_sharing{false};
    };

    struct FluidVolumeCreateInfo {
        Rx::String name;

        Uint3 size;

        // TODO: Information about what kind of fluid
    };

    struct FluidVolume {
        TextureHandle density_temperature_reaction_phi_texture_handle;
        TextureHandle velocity_pressure_texture_handle;
        Uint3 size;
        Uint32 padding;
    };

	using FluidVolumeHandle = GpuResourceHandle<FluidVolume>;

    [[nodiscard]] Uint32 size_in_bytes(TextureFormat format);

    template <typename T>
    concept GpuResource = requires(T a) {
        { a.allocation }
        ->std::convertible_to<D3D12MA::Allocation*>;
    };
} // namespace sanity::engine::renderer
