#include "d3d12_material.hpp"

#include <rx/console/interface.h>
#include <rx/core/log.h>

#include "../../core/cvar_names.hpp"

using rx::utility::move;

namespace render {
    RX_LOG("D3D12MaterialBuilder", logger);

    D3D12MaterialBuilder::D3D12MaterialBuilder(rx::memory::allocator& allocator,
                                               rx::map<rx::string, D3D12_CPU_DESCRIPTOR_HANDLE> descriptors_in)
        : internal_allocator{&allocator}, descriptors{move(descriptors_in)}, bound_buffers{allocator}, bound_images{allocator} {
        if(const auto* do_validation_slot = rx::console::interface::find_variable_by_name(ENABLE_RHI_VALIDATION_NAME)) {
            should_do_validation = do_validation_slot->cast<bool>()->get();
        }
    }

    D3D12MaterialBuilder::D3D12MaterialBuilder(D3D12MaterialBuilder&& old) noexcept
        : internal_allocator{old.internal_allocator},
          descriptors{move(old.descriptors)},
          bound_buffers{move(old.bound_buffers)},
          bound_images{move(old.bound_images)} {
        old.~D3D12MaterialBuilder();
    }

    D3D12MaterialBuilder& D3D12MaterialBuilder::operator=(D3D12MaterialBuilder&& old) noexcept {
        internal_allocator = old.internal_allocator;
        descriptors = move(old.descriptors);
        bound_buffers = move(old.bound_buffers);
        bound_images = move(old.bound_images);

        old.~D3D12MaterialBuilder();

        return *this;
    }

    MaterialBuilder& D3D12MaterialBuilder::set_buffer(const rx::string& name, const Buffer& buffer) {
        if(should_do_validation) {
            RX_ASSERT(descriptors.find(name) != nullptr,
                      "Could not bind buffer to variable %s: that variable does not exist!",
                      name.data());
        }

        const auto& d3d12_buffer = static_cast<const D3D12Buffer&>(buffer);
        if(auto* buffer_slot = bound_buffers.find(name)) {
            *buffer_slot = &d3d12_buffer;

        } else {
            bound_buffers.insert(name, &d3d12_buffer);
        }

        return *this;
    }

    MaterialBuilder& D3D12MaterialBuilder::set_image(const rx::string& name, const Image& image) {
        if(should_do_validation) {
            RX_ASSERT(descriptors.find(name) != nullptr, "Could not bind image to variable %s: that variable does not exist!", name.data());
        }

        return set_image_array(name, rx::array{&image});
    }

    MaterialBuilder& D3D12MaterialBuilder::set_image_array(const rx::string& name, const rx::vector<const Image*>& images) {
        if(should_do_validation) {
            RX_ASSERT(descriptors.find(name) != nullptr,
                      "Could not bind image array to variable %s: that variable does not exist!",
                      name.data());

            RX_ASSERT(!images.is_empty(), "Can not bind an empty image array to variable %s", name.data());
        }

        rx::vector<const D3D12Image*> d3d12_images{*internal_allocator};
        d3d12_images.reserve(images.size());

        images.each_fwd([&](const Image* image) { d3d12_images.push_back(static_cast<const D3D12Image*>(image)); });

        if(auto* image_array_slot = bound_images.find(name)) {
            *image_array_slot = move(d3d12_images);

        } else {
            bound_images.insert(name, move(d3d12_images));
        }

        return *this;
    }

    rx::ptr<Material> D3D12MaterialBuilder::build() {
        return rx::make_ptr<D3D12Material>(*internal_allocator);
    }
} // namespace render
