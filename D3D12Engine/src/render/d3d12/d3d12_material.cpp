#include "d3d12_material.hpp"

#include <rx/console/interface.h>
#include <rx/core/log.h>

#include "../../core/cvar_names.hpp"
#include "d3d12_render_device.hpp"
#include "d3dx12.hpp"

using rx::utility::move;

namespace render {
    RX_LOG("D3D12MaterialBuilder", logger);
    D3D12BindGroup::D3D12BindGroup(rx::map<UINT, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_handles_in,
                                 rx::vector<const D3D12Image*> used_images_in,
                                 rx::vector<const D3D12Buffer*> used_buffers_in)
        : descriptor_table_handles{move(descriptor_table_handles_in)},
          used_images{move(used_images_in)},
          used_buffers{move(used_buffers_in)} {}

    D3D12BindGroupBuilder::D3D12BindGroupBuilder(rx::memory::allocator& allocator,
                                               rx::map<rx::string, D3D12Descriptor> descriptors_in,
                                               rx::map<UINT, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_handles_in,
                                               D3D12RenderDevice& render_device_in)
        : internal_allocator{&allocator},
          descriptors{move(descriptors_in)},
          descriptor_table_handles{move(descriptor_table_handles_in)},
          render_device{&render_device_in},
          bound_buffers{allocator},
          bound_images{allocator} {
        if(const auto* do_validation_slot = rx::console::interface::find_variable_by_name(ENABLE_RHI_VALIDATION_NAME)) {
            should_do_validation = do_validation_slot->cast<bool>()->get();
        }
    }

    D3D12BindGroupBuilder::D3D12BindGroupBuilder(D3D12BindGroupBuilder&& old) noexcept
        : internal_allocator{old.internal_allocator},
          descriptors{move(old.descriptors)},
          descriptor_table_handles{move(old.descriptor_table_handles)},
          render_device{old.render_device},
          bound_buffers{move(old.bound_buffers)},
          bound_images{move(old.bound_images)} {
        old.~D3D12BindGroupBuilder();
    }

    D3D12BindGroupBuilder& D3D12BindGroupBuilder::operator=(D3D12BindGroupBuilder&& old) noexcept {
        internal_allocator = old.internal_allocator;
        descriptors = move(old.descriptors);
        descriptor_table_handles = move(old.descriptor_table_handles);
        bound_buffers = move(old.bound_buffers);
        bound_images = move(old.bound_images);
        render_device = old.render_device;

        old.~D3D12BindGroupBuilder();

        return *this;
    }

    BindGroupBuilder& D3D12BindGroupBuilder::set_buffer(const rx::string& name, const Buffer& buffer) {
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

    BindGroupBuilder& D3D12BindGroupBuilder::set_image(const rx::string& name, const Image& image) {
        if(should_do_validation) {
            RX_ASSERT(descriptors.find(name) != nullptr, "Could not bind image to variable %s: that variable does not exist!", name.data());
        }

        return set_image_array(name, rx::array{&image});
    }

    BindGroupBuilder& D3D12BindGroupBuilder::set_image_array(const rx::string& name, const rx::vector<const Image*>& images) {
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

    rx::ptr<BindGroup> D3D12BindGroupBuilder::build() {
        auto [images, buffers] = bind_resources_to_descriptors();

        return rx::make_ptr<D3D12BindGroup>(*internal_allocator, descriptor_table_handles, move(images), move(buffers));
    }

    rx::pair<rx::vector<const D3D12Image*>, rx::vector<const D3D12Buffer*>> D3D12BindGroupBuilder::bind_resources_to_descriptors() {
        ID3D12Device* device = render_device->get_d3d12_device();

        rx::vector<const D3D12Image*> used_images{*internal_allocator};
        rx::vector<const D3D12Buffer*> used_buffers{*internal_allocator};

        descriptors.each_pair([&](const rx::string& name, const D3D12Descriptor& descriptor) {
            if(const auto* buffer_slot = bound_buffers.find(name)) {
                const auto* buffer = *buffer_slot;
                switch(descriptor.type) {
                    case D3D12Descriptor::Type::CBV: {
                        D3D12_CONSTANT_BUFFER_VIEW_DESC desc{buffer->resource->GetGPUVirtualAddress(), static_cast<UINT>(buffer->size)};

                        device->CreateConstantBufferView(&desc, descriptor.handle);

                        used_buffers.push_back(buffer);
                    } break;

                    case D3D12Descriptor::Type::SRV: {
                        D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
                        desc.Format = DXGI_FORMAT_R8_UINT; // TODO: Figure out if that's correct
                        desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        desc.Buffer.FirstElement = 0;
                        desc.Buffer.NumElements = descriptor.num_elements;
                        desc.Buffer.StructureByteStride = descriptor.element_size;

                        device->CreateShaderResourceView(buffer->resource.Get(), &desc, descriptor.handle);

                        used_buffers.push_back(buffer);
                    } break;

                    case D3D12Descriptor::Type::UAV: {
                        D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
                        desc.Format = DXGI_FORMAT_R8_UINT; // TODO: Figure this out
                        desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                        desc.Buffer.FirstElement = 0;
                        desc.Buffer.NumElements = descriptor.num_elements;
                        desc.Buffer.StructureByteStride = descriptor.element_size;

                        device->CreateUnorderedAccessView(buffer->resource.Get(), nullptr, &desc, descriptor.handle);

                        used_buffers.push_back(buffer);
                    } break;
                }

            } else if(const auto* images_slot = bound_images.find(name)) {
                const auto& images = *images_slot;
                auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE{descriptor.handle};

                if(should_do_validation) {
                    RX_ASSERT(descriptor.type != D3D12Descriptor::Type::CBV,
                              "Can not bind a texture to constant buffer variable %s",
                              name.data());

                    RX_ASSERT(!images.is_empty(), "Can not bind an empty image array to variable %s", name.data);
                }

                if(descriptor.type == D3D12Descriptor::Type::SRV) {
                    images.each_fwd([&](const D3D12Image* image) {
                        D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
                        desc.Format = image->format;
                        desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // TODO: Figure out how to support other texture types
                        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        desc.Texture2D.MostDetailedMip = 0;
                        desc.Texture2D.MipLevels = 0xFFFFFFFF;
                        desc.Texture2D.PlaneSlice = 0;
                        desc.Texture2D.ResourceMinLODClamp = 0;

                        device->CreateShaderResourceView(image->resource.Get(), &desc, handle);
                        handle.Offset(render_device->get_shader_resource_descriptor_size());

                        used_images.push_back(image);
                    });

                } else if(descriptor.type == D3D12Descriptor::Type::UAV) {
                    images.each_fwd([&](const D3D12Image* image) {
                        D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
                        desc.Format = image->format;
                        desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                        desc.Texture2D.MipSlice = 0;
                        desc.Texture2D.PlaneSlice = 0;

                        device->CreateUnorderedAccessView(image->resource.Get(), nullptr, &desc, handle);
                        handle.Offset(render_device->get_shader_resource_descriptor_size());

                        used_images.push_back(image);
                    });
                }

            } else {
                if(should_do_validation) {
                    RX_ASSERT(false, "No resource bound for variable %s", name.data());
                }
            }
        });

        return {used_images, used_buffers};
    }
} // namespace render
