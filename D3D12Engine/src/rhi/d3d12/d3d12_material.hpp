#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include <d3d12.h>

#include "../bind_group.hpp"
#include "resources.hpp"

namespace rhi {
    class D3D12RenderDevice;

    template <typename ResourceType>
    struct BoundResource {
        BoundResource() = default;

        BoundResource(const ResourceType* resource_in, D3D12_RESOURCE_STATES states_in);

        const ResourceType* resource{nullptr};
        D3D12_RESOURCE_STATES states{};
    };

    template <typename ResourceType>
    BoundResource<ResourceType>::BoundResource(const ResourceType* resource_in, D3D12_RESOURCE_STATES states_in) : resource{resource_in}, states{states_in} {}

    struct D3D12BindGroup : BindGroup {
        explicit D3D12BindGroup(std::unordered_map<UINT, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_handles_in,
                                std::vector<BoundResource<D3D12Image>> used_images_in,
                                std::vector<BoundResource<D3D12Buffer>> used_buffers_in);

        std::unordered_map<UINT, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_handles;

        std::vector<BoundResource<D3D12Image>> used_images;
        std::vector<BoundResource<D3D12Buffer>> used_buffers;
    };

    struct D3D12Descriptor {
        enum class Type {
            CBV,
            SRV,
            UAV,
        };

        D3D12_CPU_DESCRIPTOR_HANDLE handle;

        Type type;

        /*!
         * \brief The size in bytes of one element of the array that this descriptor accesses
         *
         * This value is only meaningful if this is a SRV descriptor for a buffer
         */
        UINT element_size{0};

        /*!
         * \brief The number of array elements that this descriptor can access
         *
         * This value is only meaningful if this is a SRV descriptor for a buffer
         */
        UINT num_elements{0};
    };

    using BoundResources = std::pair<std::vector<BoundResource<D3D12Image>>, std::vector<BoundResource<D3D12Buffer>>>;

    /*!
     * \brief Abstraction for binding resources
     *
     * There's a big assumption here: no root descriptors. This makes my life easier but might need to change to enable better optimizations
     * in the future
     */
    class D3D12BindGroupBuilder final : public virtual BindGroupBuilder {
    public:
        explicit D3D12BindGroupBuilder(std::unordered_map<std::string, D3D12Descriptor> descriptors_in,
                                       std::unordered_map<UINT, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_handles_in,
                                       D3D12RenderDevice& render_device_in);

        D3D12BindGroupBuilder(const D3D12BindGroupBuilder& other) = delete;
        D3D12BindGroupBuilder& operator=(const D3D12BindGroupBuilder& other) = delete;

        D3D12BindGroupBuilder(D3D12BindGroupBuilder&& old) noexcept;
        D3D12BindGroupBuilder& operator=(D3D12BindGroupBuilder&& old) noexcept;

#pragma region BindGroupBuilder
        ~D3D12BindGroupBuilder() override = default;

        BindGroupBuilder& set_buffer(const std::string& name, const Buffer& buffer) override;

        BindGroupBuilder& set_image(const std::string& name, const Image& image) override;

        BindGroupBuilder& set_image_array(const std::string& name, const std::vector<const Image*>& images) override;

        std::unique_ptr<BindGroup> build() override;
#pragma endregion

        [[nodiscard]] BoundResources bind_resources_to_descriptors();

    private:
        std::unordered_map<std::string, D3D12Descriptor> descriptors;

        std::unordered_map<UINT, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_handles;

        D3D12RenderDevice* render_device;

        std::unordered_map<std::string, const D3D12Buffer*> bound_buffers;
        std::unordered_map<std::string, std::vector<const D3D12Image*>> bound_images;
    };
} // namespace render
