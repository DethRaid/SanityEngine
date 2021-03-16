#pragma once

#include <concepts>

#include <d3d12.h>

#include "core/types.hpp"
#include "glm/glm.hpp"
#include "per_frame_buffer.hpp"
#include "renderer/handles.hpp"
#include "renderer/rhi/per_frame_buffer.hpp"
#include "rx/core/string.h"
#include "rx/core/utility/pair.h"

namespace D3D12MA {
    class Allocation;
}

namespace sanity::engine::renderer {
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
        Uint64 size{0};
    };

    struct Buffer {
        Rx::String name;

        Uint64 size{0};

        Uint64 alignment{0};

        ComPtr<ID3D12Resource> resource;

        D3D12MA::Allocation* allocation{nullptr};

        void* mapped_ptr{nullptr};
    };

    using BufferHandle = GpuResourceHandle<Buffer>;

    class BufferRing : public ResourceRing<GpuResourceHandle<Buffer>> {
    public:
        BufferRing() = default;

        explicit BufferRing(const Rx::String& name, Uint32 size, Renderer& renderer);
    };

    enum class TextureUsage { RenderTarget, DepthStencil, SampledTexture, UnorderedAccess };

    enum class TextureFormat {
        Rgba8,
        Rg16F,
        Rgba16F,
        R32F,
        Rg32F,
        Rgba32F,
        R32UInt,
        Depth32,
        Depth24Stencil8,
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

    struct Texture {
        Rx::String name;

        Uint32 width{1};
        Uint32 height{1};
        Uint16 depth{1};

        ComPtr<ID3D12Resource> resource;

        D3D12MA::Allocation* allocation;

        TextureFormat format;
    };

    using TextureHandle = GpuResourceHandle<Texture>;

    class TextureRing : public ResourceRing<TextureHandle> {
    public:
        explicit TextureRing() = default;

        void add_texture(TextureHandle texture);
    };

    struct RenderTarget : Texture {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
    };

    struct FluidVolumeCreateInfo {
        Rx::String name;

        glm::vec3 size{1, 2, 1};

        /**
         * @brief Number of voxels per meter in this fluid volume
         *
         * @note The actual size of the voxel textures is `size * voxels_per_meter` rounded up to the nearest power of two. Thus, think of
         * this field as a suggestion
         */
        float voxels_per_meter{4};

        // TODO: Information about what kind of fluid
        // For now it's all fire
    };

    class FluidVolumeResourceRing : public ResourceRing<Rx::Pair<TextureHandle, TextureHandle>> {
    public:
        FluidVolumeResourceRing() = default;

        void add_buffer_pair(TextureHandle buffer0, TextureHandle buffer1);
    };

    struct FluidVolume {
        TextureHandle density_texture[2];

        TextureHandle temperature_texture[2];

        TextureHandle reaction_texture[2];

        TextureHandle velocity_texture[2];

        TextureHandle pressure_texture[2];

        TextureHandle temp_texture;

        glm::vec3 size{1, 2, 1};

        float voxels_per_meter{4};

        float density_dissipation{0.999f};

        float temperature_dissipation{0.995f};

        float reaction_decay{0.01f};

        float velocity_dissipation{0.995f};

        float buoyancy{0.001f};

        float weight{0.001f};

        /**
         * @brief Location of a reaction emitter, relative to the fluid volume, expressed in NDC
         */
        glm::vec3 emitter_location{0.f, 0.2f, 0.f};

        /**
         * @brief Radius of the emitter, again expressed relative to the fluid volume
         *
         * Radius of 1 means the emitter touches the sides of the volume
         *
         * Eventually we'll have support for arbitrarily shaped emitters, and multiple emitters, and really cool things that will make
         * everyone jealous
         */
        float emitter_radius{0.5f};

        float emitter_strength{1.f};

        float reaction_extinguishment{0.01f};

        float density_extinguishment_amount{1.f};

        float vorticity_strength{1.f};

        [[nodiscard]] glm::uvec3 get_voxel_size() const;
    };

    using FluidVolumeHandle = GpuResourceHandle<FluidVolume>;

    [[nodiscard]] Uint32 size_in_bytes(TextureFormat format);

    template <typename T>
    concept GpuResource = requires(T a) {
        { a.allocation }
        ->std::convertible_to<D3D12MA::Allocation*>;
    };
} // namespace sanity::engine::renderer
