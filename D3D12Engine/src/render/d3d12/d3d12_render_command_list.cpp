#include "d3d12_render_command_list.hpp"

#include <minitrace.h>

#include "../mesh_data_store.hpp"
#include "d3d12_framebuffer.hpp"
#include "d3d12_material.hpp"
#include "d3d12_render_pipeline_state.hpp"

using rx::utility::forward;
using rx::utility::move;

namespace render {
    D3D12RenderCommandList::D3D12RenderCommandList(rx::memory::allocator& allocator,
                                                   ComPtr<ID3D12GraphicsCommandList> cmds,
                                                   D3D12RenderDevice& device_in)
        : D3D12ComputeCommandList{allocator, move(cmds), device_in} {
        commands->QueryInterface(commands4.GetAddressOf());
    }

    D3D12RenderCommandList::D3D12RenderCommandList(D3D12RenderCommandList&& old) noexcept
        : D3D12ComputeCommandList{move(old)},
          commands4{move(old.commands4)},
          in_render_pass{old.in_render_pass},
          current_render_pipeline_state{old.current_render_pipeline_state},
          is_render_material_bound{old.is_render_material_bound},
          is_mesh_data_bound{old.is_mesh_data_bound} {}

    D3D12RenderCommandList& D3D12RenderCommandList::operator=(D3D12RenderCommandList&& old) noexcept {
        commands4 = old.commands4;
        in_render_pass = old.in_render_pass;
        current_render_pipeline_state = old.current_render_pipeline_state;
        is_render_material_bound = old.is_render_material_bound;
        is_mesh_data_bound = old.is_mesh_data_bound;

        return static_cast<D3D12RenderCommandList&>(D3D12ComputeCommandList::operator=(move(old)));
    }

    void D3D12RenderCommandList::set_framebuffer(const Framebuffer& framebuffer) {
        MTR_SCOPE("D3D12RenderCommandList", "set_render_targets");

        const D3D12Framebuffer& d3d12_framebuffer = static_cast<const D3D12Framebuffer&>(framebuffer);

        if(commands4) {
            // TODO: Decide if every framebuffer should be a separate renderpass

            RX_ASSERT(d3d12_framebuffer.rtv_handles.size() == d3d12_framebuffer.render_target_descriptions.size(),
                      "Render target descriptions and rtv handles must have the same length");
            RX_ASSERT(static_cast<bool>(d3d12_framebuffer.depth_stencil_desc) == static_cast<bool>(d3d12_framebuffer.dsv_handle),
                      "If a framebuffer has a depth attachment, it must have both a DSV handle and a depth_stencil description");

            if(in_render_pass) {
                commands4->EndRenderPass();
            }

            if(d3d12_framebuffer.depth_stencil_desc) {
                commands4->BeginRenderPass(static_cast<UINT>(d3d12_framebuffer.render_target_descriptions.size()),
                                           d3d12_framebuffer.render_target_descriptions.data(),
                                           &(*d3d12_framebuffer.depth_stencil_desc),
                                           D3D12_RENDER_PASS_FLAG_NONE);
            } else {
                commands4->BeginRenderPass(static_cast<UINT>(d3d12_framebuffer.render_target_descriptions.size()),
                                           d3d12_framebuffer.render_target_descriptions.data(),
                                           nullptr,
                                           D3D12_RENDER_PASS_FLAG_NONE);
            }

            in_render_pass = true;
        }

        if(d3d12_framebuffer.dsv_handle) {
            commands->OMSetRenderTargets(static_cast<UINT>(d3d12_framebuffer.rtv_handles.size()),
                                         d3d12_framebuffer.rtv_handles.data(),
                                         0,
                                         &(*d3d12_framebuffer.dsv_handle));

        } else {
            commands->OMSetRenderTargets(static_cast<UINT>(d3d12_framebuffer.rtv_handles.size()),
                                         d3d12_framebuffer.rtv_handles.data(),
                                         0,
                                         nullptr);
        }
    }

    void D3D12RenderCommandList::set_pipeline_state(const RenderPipelineState& state) {
        MTR_SCOPE("D3D12RenderCommandList", "set_pipeline_state");

        const auto& d3d12_state = static_cast<const D3D12RenderPipelineState&>(state);

        if(current_render_pipeline_state == nullptr) {
            commands->SetGraphicsRootSignature(d3d12_state.root_signature.Get());
            is_render_material_bound = false;

        } else if(current_render_pipeline_state->root_signature != d3d12_state.root_signature) {
            commands->SetGraphicsRootSignature(d3d12_state.root_signature.Get());
            is_render_material_bound = false;
        }

        commands->SetPipelineState(d3d12_state.pso.Get());

        current_render_pipeline_state = &d3d12_state;
    }

    void D3D12RenderCommandList::bind_render_resources(const BindGroup& resources) {
        MTR_SCOPE("D3D12RenderCommandList", "bind_render_resources");

        RX_ASSERT(current_render_pipeline_state != nullptr, "Must bind a render pipeline before binding render resources");

        const auto& d3d12_resources = static_cast<const D3D12BindGroup&>(resources);

        d3d12_resources.descriptor_table_handles.each_pair(
            [&](const UINT idx, const D3D12_GPU_DESCRIPTOR_HANDLE handle) { commands->SetGraphicsRootDescriptorTable(idx, handle); });

        d3d12_resources.used_buffers.each_fwd([&](const D3D12Buffer* buffer) {
            set_resource_state(*buffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        });

        d3d12_resources.used_images.each_fwd([&](const D3D12Image* image) {
            set_resource_state(*image, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        });

        is_render_material_bound = true;
    }

    void D3D12RenderCommandList::bind_mesh_data(const MeshDataStore& mesh_data) {
        MTR_SCOPE("D3D12RenderCommandList", "bind_mesh_data");

        const auto& vertex_bindings = mesh_data.get_vertex_bindings();

        // If we have more than 16 vertex attributes, we probably have bigger problems
        rx::array<D3D12_VERTEX_BUFFER_VIEW[16]> vertex_buffer_views;
        for(uint32_t i = 0; i < vertex_bindings.size(); i++) {
            const auto& binding = vertex_bindings[i];
            const auto* d3d12_buffer = static_cast<const D3D12Buffer*>(binding.buffer);

            set_resource_state(*d3d12_buffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

            D3D12_VERTEX_BUFFER_VIEW view{};
            view.BufferLocation = d3d12_buffer->resource->GetGPUVirtualAddress() + binding.offset;
            view.SizeInBytes = d3d12_buffer->size - binding.offset;
            view.StrideInBytes = binding.vertex_size;

            vertex_buffer_views[i] = view;
        }

        commands->IASetVertexBuffers(0, static_cast<UINT>(vertex_bindings.size()), vertex_buffer_views.data());

        const auto& index_buffer = mesh_data.get_index_buffer();
        const auto& d3d12_index_buffer = static_cast<const D3D12Buffer&>(index_buffer);

        set_resource_state(d3d12_index_buffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);

        D3D12_INDEX_BUFFER_VIEW index_view{};
        index_view.BufferLocation = d3d12_index_buffer.resource->GetGPUVirtualAddress();
        index_view.SizeInBytes = index_buffer.size;
        index_view.Format = DXGI_FORMAT_R32_UINT;

        commands->IASetIndexBuffer(&index_view);

        is_mesh_data_bound = true;
    }

    void D3D12RenderCommandList::draw(const uint32_t num_indices, const uint32_t first_index, const uint32_t num_instances) {
        MTR_SCOPE("D3D12RenderCommandList", "draw");

        RX_ASSERT(is_render_material_bound, "Must bind material data to issue drawcalls");
        RX_ASSERT(is_mesh_data_bound, "Must bind mesh data to issue drawcalls");
        RX_ASSERT(current_render_pipeline_state != nullptr, "Must bind a render pipeline to issue drawcalls");

        commands->DrawIndexedInstanced(num_indices, num_instances, first_index, 0, 0);
    }
} // namespace render
