#pragma once
#include <unordered_map>

#include "../bind_group.hpp"
#include "resources.hpp"

namespace rhi {
    enum class RootParameterType { Empty, Descriptor, DescriptorTable };

    enum class RootDescriptorType { ConstantBuffer, ShaderResource, UnorderedAccess };

    struct RootDescriptor {
        RootDescriptorType type{};

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

    struct D3D12BindGroup final : virtual BindGroup {
        explicit D3D12BindGroup(std::vector<RootParameter> root_parameters_in);

        ~D3D12BindGroup() override = default;

        /*!
         * \brief Binds this bind group to the active graphics root signature
         */
        void bind_to_graphics_signature(ID3D12GraphicsCommandList& cmds);

        /*!
         * \brief Binds this bind group to the active compute root signature
         */
        void bind_to_compute_signature(ID3D12GraphicsCommandList& cmds);

        std::vector<RootParameter> root_parameters;
    };

    using RootDescriptorDescription = std::pair<uint32_t, RootDescriptorType>;

    class D3D12BindGroupBuilder final : public virtual BindGroupBuilder {
    public:
        /*!
         * \brief Initializes a D3D12BindGroupBuilder with information about how to bind resources
         *
         * \param root_descriptor_descriptions_in Mapping from the string name of each root descriptor to the index and type of that
         * descriptor
         * \param descriptor_table_descriptor_mappings_in Mapping from the string name of each descriptor that's part of a descriptor table
         * to the CPU handle for that descriptor
         * \param descriptor_table_handles_in Mapping from the index of each descriptor table in the root signature to the GPU handle for
         * the start of that descriptor table
         */
        explicit D3D12BindGroupBuilder(std::unordered_map<std::string, RootDescriptorDescription> root_descriptor_descriptions_in,
                                       std::unordered_map<std::string, D3D12_CPU_DESCRIPTOR_HANDLE> descriptor_table_descriptor_mappings_in,
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
        std::unordered_map<std::string, const D3D12Buffer*> bound_buffers{};

        std::unordered_map<std::string, std::vector<const D3D12Image*>> bound_images{};

        std::unordered_map<std::string, RootDescriptorDescription> root_descriptor_descriptions;
        std::unordered_map<std::string, D3D12_CPU_DESCRIPTOR_HANDLE> descriptor_table_descriptor_mappings;
        std::unordered_map<uint32_t, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_handles;
    };
} // namespace rhi
