#pragma once

#include <d3d12.h>
#include <rx/core/map.h>

#include "../material.hpp"
#include "resources.hpp"

namespace render {
    struct D3D12Material : Material {};

    class D3D12MaterialBuilder final : public virtual MaterialBuilder {
    public:
        explicit D3D12MaterialBuilder(rx::memory::allocator& allocator, rx::map<rx::string, D3D12_CPU_DESCRIPTOR_HANDLE> descriptors_in);

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

    private:
        rx::memory::allocator* internal_allocator;

        rx::map<rx::string, D3D12_CPU_DESCRIPTOR_HANDLE> descriptors;

        rx::map<rx::string, const D3D12Buffer*> bound_buffers;
        rx::map<rx::string, rx::vector<const D3D12Image*>> bound_images;

        bool should_do_validation = false;
    };
} // namespace render
