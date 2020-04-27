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
                case RootParameterType::Resource: {
                    switch(param.resource.type) {
                        case RootResourceType::ConstantBuffer:
                            cmds.SetGraphicsRootConstantBufferView(i, param.resource.address);
                            break;

                        case RootResourceType::ShaderResource:
                            cmds.SetGraphicsRootShaderResourceView(i, param.resource.address);
                            break;

                        case RootResourceType::UnorderedAccess:
                            cmds.SetGraphicsRootUnorderedAccessView(i, param.resource.address);
                            break;
                    }
                } break;

                case RootParameterType::DescriptorTable:
                    cmds.SetGraphicsRootDescriptorTable(i, param.table.handle);
                    break;
            }
        }
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

    std::unique_ptr<BindGroup> D3D12BindGroupBuilder::build() {}
} // namespace rhi
