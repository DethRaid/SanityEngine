#pragma once
#include <unordered_map>

#include "../bind_group.hpp"
#include "resources.hpp"

namespace rhi {
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
    };

    template <typename ResourceType>
    struct BoundResource {
        BoundResource() = default;

        BoundResource(const ResourceType* resource_in, D3D12_RESOURCE_STATES states_in);

        const ResourceType* resource{nullptr};
        D3D12_RESOURCE_STATES states{};
    };

    template <typename ResourceType>
    BoundResource<ResourceType>::BoundResource(const ResourceType* resource_in, D3D12_RESOURCE_STATES states_in)
        : resource{resource_in}, states{states_in} {}


    struct D3D12BindGroup final : BindGroup {
        explicit D3D12BindGroup(std::vector<RootParameter> root_parameters_in,
                                std::vector<BoundResource<D3D12Image>> used_images_in,
                                std::vector<BoundResource<D3D12Buffer>> used_buffers_in);

        ~D3D12BindGroup() override = default;

        /*!
         * \brief Binds this bind group to the active graphics root signature
         */
        void bind_to_graphics_signature(ID3D12GraphicsCommandList& cmds) const;

        /*!
         * \brief Binds this bind group to the active compute root signature
         */
        void bind_to_compute_signature(ID3D12GraphicsCommandList& cmds) const;

        std::vector<RootParameter> root_parameters;

        std::vector<BoundResource<D3D12Image>> used_images;
        std::vector<BoundResource<D3D12Buffer>> used_buffers;
    };

    using RootDescriptorDescription = std::pair<uint32_t, DescriptorType>;

    struct DescriptorTableDescriptorDescription {
        DescriptorType type;
        D3D12_CPU_DESCRIPTOR_HANDLE handle;

        /*!
         * \brief Number of elements in the array, if this descriptor is for a structures buffer that holds an array
         */
        uint32_t num_structured_buffer_elements{1};

        /*!
         * \brief Size of one element in the structured buffer, if this binding is for a structured buffer
         */
        uint32_t structured_buffer_element_size{0};
    };

    class D3D12BindGroupBuilder final : public virtual BindGroupBuilder {
    public:
        /*!
         * \brief Initializes a D3D12BindGroupBuilder with information about how to bind resources
         *
         * \param device_in Device that will use this bind group
         * \param descriptor_size_in Size of a descriptor, used for binding image arrays
         * \param root_descriptor_descriptions_in Mapping from the string name of each root descriptor to the index and type of that
         * descriptor
         * \param descriptor_table_descriptor_mappings_in Mapping from the string name of each descriptor that's part of a descriptor table
         * to the CPU handle for that descriptor
         * \param descriptor_table_handles_in Mapping from the index of each descriptor table in the root signature to the GPU handle for
         * the start of that descriptor table
         */
        explicit D3D12BindGroupBuilder(
            ID3D12Device& device_in,
            UINT descriptor_size_in,
            std::unordered_map<std::string, RootDescriptorDescription> root_descriptor_descriptions_in,
            std::unordered_map<std::string, DescriptorTableDescriptorDescription> descriptor_table_descriptor_mappings_in,
            std::unordered_map<uint32_t, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_handles_in);

        D3D12BindGroupBuilder(const D3D12BindGroupBuilder& other) = delete;
        D3D12BindGroupBuilder& operator=(const D3D12BindGroupBuilder& other) = delete;

        D3D12BindGroupBuilder(D3D12BindGroupBuilder&& old) noexcept = default;
        D3D12BindGroupBuilder& operator=(D3D12BindGroupBuilder&& old) noexcept = default;

        ~D3D12BindGroupBuilder() override = default;

        BindGroupBuilder& set_buffer(const std::string& name, const Buffer& buffer) override;

        BindGroupBuilder& set_image(const std::string& name, const Image& image) override;

        BindGroupBuilder& set_image_array(const std::string& name, const std::vector<const Image*>& images) override;

        std::unique_ptr<BindGroup> build() override;

    private:
        ID3D12Device* device;
        UINT descriptor_size;

        std::unordered_map<std::string, const D3D12Buffer*> bound_buffers{};

        std::unordered_map<std::string, std::vector<const D3D12Image*>> bound_images{};

        std::unordered_map<std::string, RootDescriptorDescription> root_descriptor_descriptions;
        std::unordered_map<std::string, DescriptorTableDescriptorDescription> descriptor_table_descriptor_mappings;
        std::unordered_map<uint32_t, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_handles;
    };
} // namespace rhi
