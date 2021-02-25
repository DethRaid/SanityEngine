#pragma once

#include <concepts>

#include <d3d12.h>
#include <wrl/client.h>

#include "core/types.hpp"
#include "glm/glm.hpp"
#include "renderer/handles.hpp"
#include "rx/core/string.h"

namespace D3D12MA {
    class Allocation;
}

using Microsoft::WRL::ComPtr;

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

        glm::uvec3 size;

    	float voxels_per_meter{4};

        // TODO: Information about what kind of fluid
    };	

    using FluidVolumeTexture = Rx::Vector<TextureHandle[2]>;
	
    struct FluidVolume {
        FluidVolumeTexture density_texture;

    	FluidVolumeTexture temperature_texture;

    	FluidVolumeTexture reaction_texture;

    	FluidVolumeTexture velocity_texture;

    	FluidVolumeTexture pressure_texture;

        glm::uvec3 size;

        float density_dissipation{0.999f};
    	
    	float temperature_dissipation{0.995f};

    	float reaction_decay{0.01f};

    	float velocity_dissipation{0.995f};

    	float buoyancy{};

    	float weight{};
    	
    	  /**
         * @brief Location of a reaction emitter, relative to the fluid volume, expressed in NDC
         */
        glm::vec3 emitter_location{0.f, -0.8f, 0.f};

        /**
         * @brief Radius of the emitter, again expressed relative to the fluid volume
         *
         * Radius of 1 means the emitter touches the sides of the volume
         *
         * Eventually we'll have support for arbitrarily shaped emitters, and multiple emitters, and really cool things that will make
         * everyone jealous
         */
        float emitter_radius{0.1f};

    	float emitter_strength{1.f};
    	
        float reaction_extinguishment{0.01f};
    	
        float density_extinguishment_amount{1.f};
    	
        float vorticity_strength{1.f};
    };
	
    using FluidVolumeHandle = GpuResourceHandle<FluidVolume>;

    [[nodiscard]] Uint32 size_in_bytes(TextureFormat format);

    template <typename T>
    concept GpuResource = requires(T a) {
        { a.allocation }
        ->std::convertible_to<D3D12MA::Allocation*>;
    };
} // namespace sanity::engine::renderer
