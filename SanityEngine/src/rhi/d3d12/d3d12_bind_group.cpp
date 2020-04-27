#include "d3d12_bind_group.hpp"

#include <utility>

#include "../../core/ensure.hpp"
#include "minitrace.h"

namespace rhi {
    D3D12BindGroup::D3D12BindGroup(std::vector<RootParameter> root_parameters_in) : root_parameters{std::move(root_parameters_in)} {}

    void D3D12BindGroup::bind_to_graphics_signature(ID3D12GraphicsCommandList& cmds) {
        MTR_SCOPE("D3D12BindGroup", "bind_to_graphics_signature");

        for(uint32_t i = 0; i < root_parameters.size(); i++) {
            const auto& param = root_parameters[i];
            switch(param.type) {
                case RootParameterType::Descriptor: {
                    switch(param.descriptor.type) {
                        case RootDescriptorType::ConstantBuffer:
                            cmds.SetGraphicsRootConstantBufferView(i, param.descriptor.address);
                            break;

                        case RootDescriptorType::ShaderResource:
                            cmds.SetGraphicsRootShaderResourceView(i, param.descriptor.address);
                            break;

                        case RootDescriptorType::UnorderedAccess:
                            cmds.SetGraphicsRootUnorderedAccessView(i, param.descriptor.address);
                            break;
                    }
                } break;

                case RootParameterType::DescriptorTable:
                    cmds.SetGraphicsRootDescriptorTable(i, param.table.handle);
                    break;
            }
        }
    }

    void D3D12BindGroup::bind_to_compute_signature(ID3D12GraphicsCommandList& cmds) {
        MTR_SCOPE("D3D12BindGroup", "bind_to_compute_signature");

        for(uint32_t i = 0; i < root_parameters.size(); i++) {
            const auto& param = root_parameters[i];
            switch(param.type) {
                case RootParameterType::Descriptor: {
                    switch(param.descriptor.type) {
                        case RootDescriptorType::ConstantBuffer:
                            cmds.SetComputeRootConstantBufferView(i, param.descriptor.address);
                            break;

                        case RootDescriptorType::ShaderResource:
                            cmds.SetComputeRootShaderResourceView(i, param.descriptor.address);
                            break;

                        case RootDescriptorType::UnorderedAccess:
                            cmds.SetComputeRootUnorderedAccessView(i, param.descriptor.address);
                            break;
                    }
                } break;

                case RootParameterType::DescriptorTable:
                    cmds.SetComputeRootDescriptorTable(i, param.table.handle);
                    break;
            }
        }
    }

    D3D12BindGroupBuilder::D3D12BindGroupBuilder(
        std::unordered_map<std::string, RootDescriptorDescription> root_descriptor_descriptions_in,
        std::unordered_map<std::string, D3D12_CPU_DESCRIPTOR_HANDLE> descriptor_table_descriptor_mappings_in,
        std::unordered_map<uint32_t, D3D12_GPU_DESCRIPTOR_HANDLE> descriptor_table_handles_in)
        : root_descriptor_descriptions{std::move(root_descriptor_descriptions_in)},
          descriptor_table_descriptor_mappings{std::move(descriptor_table_descriptor_mappings_in)},
          descriptor_table_handles{std::move(descriptor_table_handles_in)} {
        bound_buffers.reserve(root_descriptor_descriptions.size() + descriptor_table_descriptor_mappings.size());
        bound_images.reserve(root_descriptor_descriptions.size() + descriptor_table_descriptor_mappings.size());
    }

    BindGroupBuilder& D3D12BindGroupBuilder::set_buffer(const std::string& name, const Buffer& buffer) {
        auto& d3d12_buffer = static_cast<const D3D12Buffer&>(buffer);
        bound_buffers.insert_or_assign(name, &d3d12_buffer);

        return *this;
    }

    BindGroupBuilder& D3D12BindGroupBuilder::set_image(const std::string& name, const Image& image) {
        return set_image_array(name, {&image});
    }

    BindGroupBuilder& D3D12BindGroupBuilder::set_image_array(const std::string& name, const std::vector<const Image*>& images) {
        std::vector<const D3D12Image*> d3d12_images;
        d3d12_images.reserve(images.size());

        for(const auto* image : images) {
            d3d12_images.push_back(static_cast<const D3D12Image*>(image));
        }

        bound_buffers.insert_or_assign(name, d3d12_images);

        return *this;
    }

    std::unique_ptr<BindGroup> D3D12BindGroupBuilder::build() {
        MTR_SCOPE("D3D12BindGroupBuilder", "build");
    }
} // namespace rhi
