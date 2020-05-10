#include "bind_group.hpp"

#include <utility>

#include <minitrace.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "../core/ensure.hpp"
#include "d3dx12.hpp"
#include "helpers.hpp"
#include "raytracing_structs.hpp"

namespace rhi {
    RootParameter::RootParameter() = default;

    BindGroup::BindGroup(ID3D12DescriptorHeap& heap_in,
                         std::vector<RootParameter> root_parameters_in,
                         std::vector<BoundResource<Image>> used_images_in,
                         std::vector<BoundResource<Buffer>> used_buffers_in)
        : heap{&heap_in},
          root_parameters{std::move(root_parameters_in)},
          used_images{std::move(used_images_in)},
          used_buffers{std::move(used_buffers_in)} {}

    void BindGroup::bind_to_graphics_signature(ID3D12GraphicsCommandList& cmds) const {
        MTR_SCOPE("BindGroup", "bind_to_graphics_signature");

        for(uint32_t i = 0; i < root_parameters.size(); i++) {
            const auto& param = root_parameters[i];
            if(param.type == RootParameterType::Descriptor) {
                switch(param.descriptor.type) {
                    case DescriptorType::ConstantBuffer:
                        cmds.SetGraphicsRootConstantBufferView(i, param.descriptor.address);
                        break;

                    case DescriptorType::ShaderResource:
                        cmds.SetGraphicsRootShaderResourceView(i, param.descriptor.address);
                        break;

                    case DescriptorType::UnorderedAccess:
                        cmds.SetGraphicsRootUnorderedAccessView(i, param.descriptor.address);
                        break;
                }

            } else if(param.type == RootParameterType::DescriptorTable) {
                cmds.SetGraphicsRootDescriptorTable(i, param.table.handle);
            }
        }
    }

    void BindGroup::bind_to_compute_signature(ID3D12GraphicsCommandList& cmds) const {
        MTR_SCOPE("BindGroup", "bind_to_compute_signature");

        for(uint32_t i = 0; i < root_parameters.size(); i++) {
            const auto& param = root_parameters[i];
            if(param.type == RootParameterType::Descriptor) {
                switch(param.descriptor.type) {
                    case DescriptorType::ConstantBuffer:
                        cmds.SetComputeRootConstantBufferView(i, param.descriptor.address);
                        break;

                    case DescriptorType::ShaderResource:
                        cmds.SetComputeRootShaderResourceView(i, param.descriptor.address);
                        break;

                    case DescriptorType::UnorderedAccess:
                        cmds.SetComputeRootUnorderedAccessView(i, param.descriptor.address);
                        break;
                }

            } else if(param.type == RootParameterType::DescriptorTable) {
                cmds.SetComputeRootDescriptorTable(i, param.table.handle);
            }
        }
    }

    BindGroupBuilder::BindGroupBuilder(
        ID3D12Device& device_in,
        ID3D12DescriptorHeap& heap_in,
        const UINT descriptor_size_in,
        std::unordered_map<std::string, RootDescriptorDescription> root_descriptor_descriptions_in,
        std::unordered_map<std::string, DescriptorTableDescriptorDescription> descriptor_table_descriptor_mappings_in,
        std::unordered_map<uint32_t, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_handles_in)
        : device{&device_in},
          heap{&heap_in},
          descriptor_size{descriptor_size_in},
          root_descriptor_descriptions{std::move(root_descriptor_descriptions_in)},
          descriptor_table_descriptor_mappings{std::move(descriptor_table_descriptor_mappings_in)},
          descriptor_table_handles{std::move(descriptor_table_handles_in)} {
        if(const auto loggy = spdlog::get("BindGroupBuilder")) {
            logger = loggy;

        } else {
            logger = spdlog::stdout_color_st("BindGroupBuilder");
        }

        bound_buffers.reserve(root_descriptor_descriptions.size() + descriptor_table_descriptor_mappings.size());
        bound_images.reserve(root_descriptor_descriptions.size() + descriptor_table_descriptor_mappings.size());
    }

    BindGroupBuilder& BindGroupBuilder::set_buffer(const std::string& name, const Buffer& buffer) {
        const auto& d3d12_buffer = static_cast<const Buffer&>(buffer);
        bound_buffers.insert_or_assign(name, &d3d12_buffer);

        return *this;
    }

    BindGroupBuilder& BindGroupBuilder::set_image(const std::string& name, const Image& image) { return set_image_array(name, {&image}); }

    BindGroupBuilder& BindGroupBuilder::set_image_array(const std::string& name, const std::vector<const Image*>& images) {
        std::vector<const Image*> d3d12_images;
        d3d12_images.reserve(images.size());

        for(const auto* image : images) {
            d3d12_images.push_back(static_cast<const Image*>(image));
        }

        bound_images.insert_or_assign(name, d3d12_images);

        return *this;
    }

    BindGroupBuilder& BindGroupBuilder::set_raytracing_scene(const std::string& name, const RaytracingScene& scene) {
        const auto* d3d12_buffer = static_cast<const Buffer*>(scene.buffer.get());
        bound_raytracing_scenes.emplace(name, d3d12_buffer);

        return *this;
    }

    std::unique_ptr<BindGroup> BindGroupBuilder::build() {
        MTR_SCOPE("BindGroupBuilder", "build");

        // D3D12 has a maximum root signature size of 64 descriptor tables
        std::vector<RootParameter> root_parameters{64};

        // Save descriptor table information
        for(const auto& [idx, handle] : descriptor_table_handles) {
            ENSURE(idx < 64, "May not have more than 64 descriptor tables in a single bind group");

            root_parameters[idx].type = RootParameterType::DescriptorTable;
            root_parameters[idx].table.handle = handle;
        }

        std::vector<BoundResource<Image>> used_images;
        std::vector<BoundResource<Buffer>> used_buffers;

        // Save root descriptor information
        for(const auto& [name, desc] : root_descriptor_descriptions) {
            const auto& [idx, type] = desc;
            ENSURE(idx < 32, "May not have more than 32 root descriptors in a single bind group");

            ENSURE(root_parameters[idx].type == RootParameterType::Empty, "Root parameter index {} already used", idx);

            root_parameters[idx].type = RootParameterType::Descriptor;
            root_parameters[idx].descriptor.type = type;

            if(const auto& buffer_itr = bound_buffers.find(name); buffer_itr != bound_buffers.end()) {
                root_parameters[idx].descriptor.address = buffer_itr->second->resource->GetGPUVirtualAddress();
                logger->trace("Binding buffer {} to root descriptor {}", buffer_itr->second->name, name);

                auto states = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
                if(type == DescriptorType::ConstantBuffer) {
                    states |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

                } else if(type == DescriptorType::UnorderedAccess) {
                    states |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                }

                used_buffers.emplace_back(buffer_itr->second, states);

            } else if(const auto& image_itr = bound_images.find(name); image_itr != bound_images.end()) {
                ENSURE(image_itr->second.size() == 1, "May only bind a single image to a root descriptor");
                root_parameters[idx].descriptor.address = image_itr->second[0]->resource->GetGPUVirtualAddress();

                auto states = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
                if(type == DescriptorType::ConstantBuffer) {
                    states |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

                } else if(type == DescriptorType::UnorderedAccess) {
                    states |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                }

                used_images.emplace_back(image_itr->second[0], states);

            } else if(const auto& scene_itr = bound_raytracing_scenes.find(name); scene_itr != bound_raytracing_scenes.end()) {
                ENSURE(type == DescriptorType::ShaderResource,
                       "May only bind raytracing acceleration structure {} as a shader resource",
                       scene_itr->second->name);

                const auto* scene = scene_itr->second;

                root_parameters[idx].descriptor.address = scene->resource->GetGPUVirtualAddress();

                // Don't need to issue barriers for raytracing acceleration structures

            } else {
                logger->warn("No resources bound to root descriptor {}", name);
            }
        }

        // Bind resources to descriptor table descriptors
        for(const auto& [name, desc] : descriptor_table_descriptor_mappings) {
            if(const auto& buffer_itr = bound_buffers.find(name); buffer_itr != bound_buffers.end()) {
                auto* buffer = buffer_itr->second;

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

            } else if(const auto& image_itr = bound_images.find(name); image_itr != bound_images.end()) {
                switch(desc.type) {
                    case DescriptorType::ConstantBuffer: {
                        logger->warn("Can not bind images to constant buffer {}", name);
                    } break;

                    case DescriptorType::ShaderResource: {
                        ENSURE(desc.num_structured_buffer_elements == 1, "Cannot bind an image to structure array {}", name);
                        ENSURE(desc.structured_buffer_element_size == 0, "Cannot bind an image to structure array {}", name);

                        CD3DX12_CPU_DESCRIPTOR_HANDLE handle{desc.handle};

                        for(const auto* image : image_itr->second) {
                            D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
                            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                            srv_desc.Format = to_dxgi_format(image->format);
                            srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                            srv_desc.Texture2D.MostDetailedMip = 0;
                            srv_desc.Texture2D.MipLevels = 0xFFFFFFFF;
                            srv_desc.Texture2D.PlaneSlice = 0;
                            srv_desc.Texture2D.ResourceMinLODClamp = 0;

                            device->CreateShaderResourceView(image->resource.Get(), &srv_desc, handle);

                            handle.Offset(descriptor_size);
                        }
                    } break;

                    case DescriptorType::UnorderedAccess: {
                        CD3DX12_CPU_DESCRIPTOR_HANDLE handle{desc.handle};

                        for(const auto* image : image_itr->second) {
                            D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
                            uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                            uav_desc.Format = to_dxgi_format(image->format);
                            uav_desc.Texture2D.MipSlice = 0;
                            uav_desc.Texture2D.PlaneSlice = 0;

                            device->CreateUnorderedAccessView(image->resource.Get(), nullptr, &uav_desc, handle);

                            handle.Offset(descriptor_size);
                        }
                    } break;
                }
            } else {
                logger->warn("No resource bound to descriptor {}", name);
            }
        }

        return std::make_unique<BindGroup>(*heap, std::move(root_parameters), std::move(used_images), std::move(used_buffers));
    }
} // namespace rhi
