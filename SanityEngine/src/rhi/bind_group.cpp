#include "bind_group.hpp"

#include <Tracy.hpp>
#include <rx/core/log.h>

#include "rhi/d3dx12.hpp"
#include "rhi/helpers.hpp"
#include "rhi/raytracing_structs.hpp"

namespace renderer {
    RX_LOG("BindGroupBuilder", logger);

    RootParameter::RootParameter() = default;

    BindGroup::BindGroup(ID3D12DescriptorHeap& heap_in,
                         Rx::Vector<RootParameter> root_parameters_in,
                         Rx::Vector<BoundResource<Image>> used_images_in,
                         Rx::Vector<BoundResource<Buffer>> used_buffers_in)
        : heap{&heap_in},
          root_parameters{Rx::Utility::move(root_parameters_in)},
          used_images{Rx::Utility::move(used_images_in)},
          used_buffers{Rx::Utility::move(used_buffers_in)} {}

    void BindGroup::bind_to_graphics_signature(ComPtr<ID3D12GraphicsCommandList> cmds) const {
        cmds->SetDescriptorHeaps(1, &heap);

        for(Uint32 i = 0; i < root_parameters.size(); i++) {
            const auto& param = root_parameters[i];
            if(param.type == RootParameterType::Descriptor) {
                switch(param.descriptor.type) {
                    case DescriptorType::ConstantBuffer:
                        cmds->SetGraphicsRootConstantBufferView(i, param.descriptor.address);
                        break;

                    case DescriptorType::ShaderResource:
                        cmds->SetGraphicsRootShaderResourceView(i, param.descriptor.address);
                        break;

                    case DescriptorType::UnorderedAccess:
                        cmds->SetGraphicsRootUnorderedAccessView(i, param.descriptor.address);
                        break;
                }

            } else if(param.type == RootParameterType::DescriptorTable) {
                cmds->SetGraphicsRootDescriptorTable(i, param.table.handle);
            }
        }
    }

    void BindGroup::bind_to_compute_signature(ComPtr<ID3D12GraphicsCommandList> cmds) const {
        cmds->SetDescriptorHeaps(1, &heap);

        for(Uint32 i = 0; i < root_parameters.size(); i++) {
            const auto& param = root_parameters[i];
            if(param.type == RootParameterType::Descriptor) {
                switch(param.descriptor.type) {
                    case DescriptorType::ConstantBuffer:
                        cmds->SetComputeRootConstantBufferView(i, param.descriptor.address);
                        break;

                    case DescriptorType::ShaderResource:
                        cmds->SetComputeRootShaderResourceView(i, param.descriptor.address);
                        break;

                    case DescriptorType::UnorderedAccess:
                        cmds->SetComputeRootUnorderedAccessView(i, param.descriptor.address);
                        break;
                }

            } else if(param.type == RootParameterType::DescriptorTable) {
                cmds->SetComputeRootDescriptorTable(i, param.table.handle);
            }
        }
    }

    BindGroupBuilder::BindGroupBuilder(ID3D12Device& device_in,
                                       ID3D12DescriptorHeap& heap_in,
                                       const UINT descriptor_size_in,
                                       Rx::Map<Rx::String, RootDescriptorDescription> root_descriptor_descriptions_in,
                                       Rx::Map<Rx::String, DescriptorTableDescriptorDescription> descriptor_table_descriptor_mappings_in,
                                       Rx::Map<Uint32, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_handles_in)
        : device{&device_in},
          heap{&heap_in},
          descriptor_size{descriptor_size_in},
          root_descriptor_descriptions{Rx::Utility::move(root_descriptor_descriptions_in)},
          descriptor_table_descriptor_mappings{Rx::Utility::move(descriptor_table_descriptor_mappings_in)},
          descriptor_table_handles{Rx::Utility::move(descriptor_table_handles_in)} {}

    void BindGroupBuilder::clear_all_bindings() {
        bound_buffers = {};
        bound_image_arrays = {};
        bound_raytracing_scenes = {};
    }

    BindGroupBuilder& BindGroupBuilder::set_buffer(const Rx::String& name, const Buffer& buffer) {
        ZoneScoped;

        const auto& d3d12_buffer = static_cast<const Buffer&>(buffer);
        bound_buffers.insert(name, &d3d12_buffer);

        return *this;
    }

    BindGroupBuilder& BindGroupBuilder::set_image(const Rx::String& name, const Image& image) {
        return set_image_array(name, Rx::Array{&image});
    }

    BindGroupBuilder& BindGroupBuilder::set_image_array(const Rx::String& name, const Rx::Vector<const Image*>& images) {
        ZoneScoped;

        bound_image_arrays.insert(name, images);

        return *this;
    }

    BindGroupBuilder& BindGroupBuilder::set_raytracing_scene(const Rx::String& name, const RaytracingScene& scene) {
        ZoneScoped;

        bound_raytracing_scenes.insert(name, scene.buffer.get());

        return *this;
    }

    Rx::Ptr<BindGroup> BindGroupBuilder::build() {
        ZoneScoped;
        // D3D12 has a maximum root signature size of 64 descriptor tables
        Rx::Vector<RootParameter> root_parameters{64};

        // Save descriptor table information
        descriptor_table_handles.each_pair([&](const Uint32 idx, const D3D12_GPU_DESCRIPTOR_HANDLE& handle) {
            RX_ASSERT(idx < 64, "May not have more than 64 descriptor tables in a single bind group");

            root_parameters[idx].type = RootParameterType::DescriptorTable;
            root_parameters[idx].table.handle = handle;
        });

        Rx::Vector<BoundResource<Image>> used_images;
        Rx::Vector<BoundResource<Buffer>> used_buffers;

        // Save root descriptor information
        root_descriptor_descriptions.each_pair([&](const Rx::String& name, const RootDescriptorDescription& desc) {
            const auto& [idx, type] = desc;
            RX_ASSERT(idx < 32, "May not have more than 32 root descriptors in a single bind group");

            RX_ASSERT(root_parameters[idx].type == RootParameterType::Empty, "Root parameter index %d already used", idx);

            root_parameters[idx].type = RootParameterType::Descriptor;
            root_parameters[idx].descriptor.type = type;

            if(const Buffer** bound_buffer = bound_buffers.find(name)) {
                root_parameters[idx].descriptor.address = (*bound_buffer)->resource->GetGPUVirtualAddress();

                auto states = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
                if(type == DescriptorType::ConstantBuffer) {
                    states |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

                } else if(type == DescriptorType::UnorderedAccess) {
                    states |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                }

                used_buffers.emplace_back(*bound_buffer, states);

            } else if(const Rx::Vector<const Image*>* bound_image = bound_image_arrays.find(name)) {
                RX_ASSERT(bound_image->size() == 1, "May only bind a single image to a root descriptor");
                root_parameters[idx].descriptor.address = (*bound_image)[0]->resource->GetGPUVirtualAddress();

                auto states = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
                if(type == DescriptorType::ConstantBuffer) {
                    states |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

                } else if(type == DescriptorType::UnorderedAccess) {
                    states |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                }

                used_images.emplace_back((*bound_image)[0], states);

            } else if(const Buffer** scene = bound_raytracing_scenes.find(name)) {
                RX_ASSERT(type == DescriptorType::ShaderResource,
                          "May only bind raytracing acceleration structure %s as a shader resource",
                          (*scene)->name.data());

                root_parameters[idx].descriptor.address = (*scene)->resource->GetGPUVirtualAddress();

                // Don't need to issue barriers for raytracing acceleration structures

            } else {
                // logger->warn("No resources bound to root descriptor {}", name);
            }
        });

        // Bind resources to descriptor table descriptors
        descriptor_table_descriptor_mappings.each_pair([&](const Rx::String& name, const DescriptorTableDescriptorDescription& desc) {
            if(const Buffer** buffer_itr = bound_buffers.find(name)) {
                auto* buffer = *buffer_itr;

                auto states = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

                switch(desc.type) {
                    case DescriptorType::ConstantBuffer: {
                        D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc{};
                        cbv_desc.SizeInBytes = buffer->size;
                        cbv_desc.BufferLocation = buffer->resource->GetGPUVirtualAddress();

                        device->CreateConstantBufferView(&cbv_desc, desc.handle);

                        states |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
                    } break;

                    case DescriptorType::ShaderResource: {
                        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
                        srv_desc.Format = DXGI_FORMAT_R8_UNORM;
                        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        srv_desc.Buffer.FirstElement = 0;
                        srv_desc.Buffer.NumElements = desc.num_structured_buffer_elements;
                        srv_desc.Buffer.StructureByteStride = desc.structured_buffer_element_size;
                        srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

                        device->CreateShaderResourceView(buffer->resource.Get(), &srv_desc, desc.handle);
                    } break;

                    case DescriptorType::UnorderedAccess: {
                        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
                        uav_desc.Format = DXGI_FORMAT_R8_UNORM;
                        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                        uav_desc.Buffer.FirstElement = 0;
                        uav_desc.Buffer.NumElements = desc.num_structured_buffer_elements;
                        uav_desc.Buffer.StructureByteStride = desc.structured_buffer_element_size;
                        uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

                        device->CreateUnorderedAccessView(buffer->resource.Get(), nullptr, &uav_desc, desc.handle);

                        states |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                    } break;
                }

                used_buffers.emplace_back(buffer, states);

            } else if(const auto* images = bound_image_arrays.find(name)) {
                switch(desc.type) {
                    case DescriptorType::ConstantBuffer: {
                        logger->warning("Can not bind images to constant buffer %s", name);
                    } break;

                    case DescriptorType::ShaderResource: {
                        RX_ASSERT(desc.num_structured_buffer_elements == 1, "Cannot bind an image to structure array %s", name.data());
                        RX_ASSERT(desc.structured_buffer_element_size == 0, "Cannot bind an image to structure array %s", name.data());

                        CD3DX12_CPU_DESCRIPTOR_HANDLE handle{desc.handle};

                        images->each_fwd([&](const Image* image) {
                            if(image != nullptr) {
                                D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
                                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                                srv_desc.Format = to_dxgi_format(image->format);
                                if(srv_desc.Format == DXGI_FORMAT_D32_FLOAT) {
                                    srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
                                }
                                srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                                srv_desc.Texture2D.MostDetailedMip = 0;
                                srv_desc.Texture2D.MipLevels = 0xFFFFFFFF;
                                srv_desc.Texture2D.PlaneSlice = 0;
                                srv_desc.Texture2D.ResourceMinLODClamp = 0;

                                device->CreateShaderResourceView(image->resource.Get(), &srv_desc, handle);
                            }

                            handle.Offset(descriptor_size);
                        });
                    } break;

                    case DescriptorType::UnorderedAccess: {
                        CD3DX12_CPU_DESCRIPTOR_HANDLE handle{desc.handle};

                        images->each_fwd([&](const Image* image) {
                            const auto uav_desc = D3D12_UNORDERED_ACCESS_VIEW_DESC{.Format = to_dxgi_format(image->format),
                                                                                   .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
                                                                                   .Texture2D = {
                                                                                       .MipSlice = 0,
                                                                                       .PlaneSlice = 0,
                                                                                   }};

                            device->CreateUnorderedAccessView(image->resource.Get(), nullptr, &uav_desc, handle);

                            handle.Offset(descriptor_size);
                        });
                    } break;
                }
            } else {
                // logger->warn("No resource bound to descriptor {}", name);
            }
        });

        return Rx::make_ptr<BindGroup>(Rx::Memory::SystemAllocator::instance(),
                                       *heap,
                                       Rx::Utility::move(root_parameters),
                                       Rx::Utility::move(used_images),
                                       Rx::Utility::move(used_buffers));
    } // namespace rhi
} // namespace renderer
