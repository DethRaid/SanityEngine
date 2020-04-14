#pragma once

#include <d3d12.h>
#include <rx/core/map.h>

#include "../bind_group.hpp"
#include "resources.hpp"

namespace render {
    class D3D12RenderDevice;

    template <typename ResourceType>
    struct BoundResource {
        const ResourceType* resource;
        D3D12_RESOURCE_STATES states;
    };

    struct D3D12BindGroup : BindGroup {
        explicit D3D12BindGroup(rx::map<UINT, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_handles_in,
                                rx::vector<const D3D12Image*> used_images_in,
                                rx::vector<const D3D12Buffer*> used_buffers_in);

        rx::map<UINT, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_handles;

        rx::vector<BoundResource<D3D12Image>> used_images;
        rx::vector<BoundResource<D3D12Buffer>> used_buffers;
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

    using BoundResources = rx::pair<rx::vector<BoundResource<D3D12Image>>, rx::vector<BoundResource<D3D12Buffer>>>;

    /*!
     * \brief Abstraction for binding resources
     *
     * There's a big assumption here: no root descriptors. This makes my life easier but might need to change to enable better optimizations
     * in the future
     */
    class D3D12BindGroupBuilder final : public virtual BindGroupBuilder {
    public:
        explicit D3D12BindGroupBuilder(rx::memory::allocator& allocator,
                                       rx::map<rx::string, D3D12Descriptor> descriptors_in,
                                       rx::map<UINT, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_handles_in,
                                       D3D12RenderDevice& render_device_in);

        D3D12BindGroupBuilder(const D3D12BindGroupBuilder& other) = delete;
        D3D12BindGroupBuilder& operator=(const D3D12BindGroupBuilder& other) = delete;

        D3D12BindGroupBuilder(D3D12BindGroupBuilder&& old) noexcept;
        D3D12BindGroupBuilder& operator=(D3D12BindGroupBuilder&& old) noexcept;

#pragma region MaterialBuilder
        ~D3D12BindGroupBuilder() override = default;

        BindGroupBuilder& set_buffer(const rx::string& name, const Buffer& buffer) override;

        BindGroupBuilder& set_image(const rx::string& name, const Image& image) override;

        BindGroupBuilder& set_image_array(const rx::string& name, const rx::vector<const Image*>& images) override;

        rx::ptr<BindGroup> build() override;
#pragma endregion

        [[nodiscard]] BoundResources bind_resources_to_descriptors();

    private:
        rx::memory::allocator* internal_allocator;

        rx::map<rx::string, D3D12Descriptor> descriptors;

        rx::map<UINT, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_handles;

        D3D12RenderDevice* render_device;

        rx::map<rx::string, const D3D12Buffer*> bound_buffers;
        rx::map<rx::string, rx::vector<const D3D12Image*>> bound_images;

        bool should_do_validation = false;
    };
} // namespace render
