#pragma once
#include <unordered_map>

#include "../bind_group.hpp"
#include "resources.hpp"

namespace rhi {
    enum class RootParameterType { Resource, DescriptorTable };

    enum class RootResourceType { ConstantBuffer, ShaderResource, UnorderedAccess };

    struct RootResource {
        RootResourceType type{RootResourceType::ConstantBuffer};

        D3D12_GPU_VIRTUAL_ADDRESS address;
    };

    struct RootDescriptorTable {
        D3D12_GPU_DESCRIPTOR_HANDLE handle{};
    };

    struct RootParameter {
        RootParameterType type{RootParameterType::Resource};

        union {
            RootResource resource{};
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

    class D3D12BindGroupBuilder final : public virtual BindGroupBuilder {
    public:
        explicit D3D12BindGroupBuilder();

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
        std::unordered_map<std::string, const D3D12Buffer*> bound_buffers;

        std::unordered_map<std::string, std::vector<const D3D12Image*>> bound_images;
    };
} // namespace rhi
