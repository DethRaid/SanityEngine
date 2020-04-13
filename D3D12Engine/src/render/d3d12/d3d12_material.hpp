#pragma once

#include <d3d12.h>
#include <rx/core/map.h>

#include "../material.hpp"
#include "resources.hpp"

namespace render {
    class D3D12RenderDevice;

    struct D3D12Material : Material {};

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

    class D3D12MaterialBuilder final : public virtual MaterialBuilder {
    public:
        explicit D3D12MaterialBuilder(rx::memory::allocator& allocator,
                                      rx::map<rx::string, D3D12Descriptor> descriptors_in,
                                      D3D12RenderDevice& render_device_in);

        D3D12MaterialBuilder(const D3D12MaterialBuilder& other) = delete;
        D3D12MaterialBuilder& operator=(const D3D12MaterialBuilder& other) = delete;

        D3D12MaterialBuilder(D3D12MaterialBuilder&& old) noexcept;
        D3D12MaterialBuilder& operator=(D3D12MaterialBuilder&& old) noexcept;

#pragma region MaterialBuilder
        ~D3D12MaterialBuilder() override = default;

        MaterialBuilder& set_buffer(const rx::string& name, const Buffer& buffer) override;

        MaterialBuilder& set_image(const rx::string& name, const Image& image) override;

        MaterialBuilder& set_image_array(const rx::string& name, const rx::vector<const Image*>& images) override;

        rx::ptr<Material> build() override;
#pragma endregion

        void update_descriptors();

    private:
        rx::memory::allocator* internal_allocator;

        rx::map<rx::string, D3D12Descriptor> descriptors;

        D3D12RenderDevice* render_device;

        rx::map<rx::string, const D3D12Buffer*> bound_buffers;
        rx::map<rx::string, rx::vector<const D3D12Image*>> bound_images;

        bool should_do_validation = false;
    };
} // namespace render
