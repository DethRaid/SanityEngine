#pragma once

#include <d3d12.h>
#include <rx/core/map.h>
#include <rx/core/ptr.h>
#include <rx/core/string.h>
#include <rx/core/vector.h>
#include <wrl/client.h>

#include "core/types.hpp"

using Microsoft::WRL::ComPtr;

namespace renderer {
    struct RaytracingScene;
    struct Buffer;
    struct Image;

    enum class RootParameterType { Empty, Descriptor, DescriptorTable };

    enum class DescriptorType { ConstantBuffer, ShaderResource, UnorderedAccess };

    struct RootDescriptor {
        DescriptorType type{};

        D3D12_GPU_VIRTUAL_ADDRESS address;
    };

    struct RootDescriptorTable {
        D3D12_GPU_DESCRIPTOR_HANDLE handle{};
    };

    struct RootParameter {
        RootParameterType type{};

        union {
            RootDescriptor descriptor{};
            RootDescriptorTable table;
        };

        RootParameter();
    };

    template <typename ResourceType>
    struct BoundResource {
        BoundResource() = default;

        BoundResource(const ResourceType* resource_in, D3D12_RESOURCE_STATES states_in);

        const ResourceType* resource{nullptr};
        D3D12_RESOURCE_STATES states{};
    };

    template <typename ResourceType>
    BoundResource<ResourceType>::BoundResource(const ResourceType* resource_in, const D3D12_RESOURCE_STATES states_in)
        : resource{resource_in}, states{states_in} {}

    struct BindGroup {
        explicit BindGroup(ID3D12DescriptorHeap& heap_in,
                           Rx::Vector<RootParameter> root_parameters_in,
                           Rx::Vector<BoundResource<Image>> used_images_in,
                           Rx::Vector<BoundResource<Buffer>> used_buffers_in);

        BindGroup(const BindGroup& other) = default;
        BindGroup& operator=(const BindGroup& other) = default;

        BindGroup(BindGroup&& old) noexcept = default;
        BindGroup& operator=(BindGroup&& old) noexcept = default;

        ~BindGroup() = default;

        /*!
         * \brief Binds this bind group to the active graphics root signature
         */
        void bind_to_graphics_signature(ComPtr<ID3D12GraphicsCommandList> cmds) const;

        /*!
         * \brief Binds this bind group to the active compute root signature
         */
        void bind_to_compute_signature(ComPtr<ID3D12GraphicsCommandList> cmds) const;

        ID3D12DescriptorHeap* heap;

        Rx::Vector<RootParameter> root_parameters;

        Rx::Vector<BoundResource<Image>> used_images;
        Rx::Vector<BoundResource<Buffer>> used_buffers;
    };

    using RootDescriptorDescription = Rx::Pair<Uint32, DescriptorType>;

    struct DescriptorTableDescriptorDescription {
        DescriptorType type;
        D3D12_CPU_DESCRIPTOR_HANDLE handle;

        /*!
         * \brief Number of elements in the array, if this descriptor is for a structures buffer that holds an array
         */
        Uint32 num_structured_buffer_elements{1};

        /*!
         * \brief Size of one element in the structured buffer, if this binding is for a structured buffer
         */
        Uint32 structured_buffer_element_size{0};
    };

    class BindGroupBuilder {
    public:
        /*!
         * \brief Initializes a BindGroupBuilder with information about how to bind resources
         *
         * \param device_in Device that will use this bind group
         * \param heap_in The descriptor heap that this bind group builder puts descriptors in
         * \param descriptor_size_in Size of a descriptor, used for binding image arrays
         * \param root_descriptor_descriptions_in Mapping from the string name of each root descriptor to the index and type of that
         * descriptor
         * \param descriptor_table_descriptor_mappings_in Mapping from the string name of each descriptor that's part of a descriptor table
         * to the CPU handle for that descriptor
         * \param descriptor_table_handles_in Mapping from the index of each descriptor table in the root signature to the GPU handle for
         * the start of that descriptor table
         */
        explicit BindGroupBuilder(ID3D12Device& device_in,
                                  ID3D12DescriptorHeap& heap_in,
                                  UINT descriptor_size_in,
                                  Rx::Map<Rx::String, RootDescriptorDescription> root_descriptor_descriptions_in,
                                  Rx::Map<Rx::String, DescriptorTableDescriptorDescription> descriptor_table_descriptor_mappings_in,
                                  Rx::Map<Uint32, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_handles_in);

        BindGroupBuilder(const BindGroupBuilder& other) = default;
        BindGroupBuilder& operator=(const BindGroupBuilder& other) = default;

        BindGroupBuilder(BindGroupBuilder&& old) noexcept = default;
        BindGroupBuilder& operator=(BindGroupBuilder&& old) noexcept = default;

        ~BindGroupBuilder() = default;

        void clear_all_bindings();

        BindGroupBuilder& set_buffer(const Rx::String& name, const Buffer& buffer);

        BindGroupBuilder& set_image(const Rx::String& name, const Image& image);

        BindGroupBuilder& set_image_array(const Rx::String& name, const Rx::Vector<const Image*>& images);

        BindGroupBuilder& set_raytracing_scene(const Rx::String& name, const RaytracingScene& scene);

        Rx::Ptr<BindGroup> build();

    private:
        ID3D12Device* device;

        ID3D12DescriptorHeap* heap;

        UINT descriptor_size;

        Rx::Map<Rx::String, const Buffer*> bound_buffers{};

        Rx::Map<Rx::String, Rx::Vector<const Image*>> bound_image_arrays{};

        Rx::Map<Rx::String, const Buffer*> bound_raytracing_scenes{};

        Rx::Map<Rx::String, RootDescriptorDescription> root_descriptor_descriptions;
        Rx::Map<Rx::String, DescriptorTableDescriptorDescription> descriptor_table_descriptor_mappings;
        Rx::Map<Uint32, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_handles;
    };
} // namespace renderer
