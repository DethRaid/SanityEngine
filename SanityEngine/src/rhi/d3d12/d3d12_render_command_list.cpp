#include "d3d12_render_command_list.hpp"

#include <array>
#include <cassert>

#include <minitrace.h>

#include "../../core/ensure.hpp"
#include "../mesh_data_store.hpp"
#include "d3d12_bind_group.hpp"
#include "d3d12_framebuffer.hpp"
#include "d3d12_render_pipeline_state.hpp"
#include "helpers.hpp"

using std::move;

namespace rhi {
    D3D12RenderCommandList::D3D12RenderCommandList(ComPtr<ID3D12GraphicsCommandList> cmds, D3D12RenderDevice& device_in)
        : D3D12ComputeCommandList{move(cmds), device_in} {
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

    void D3D12RenderCommandList::set_framebuffer(const Framebuffer& framebuffer,
                                                 std::vector<RenderTargetAccess> render_target_accesses,
                                                 std::optional<RenderTargetAccess> depth_access) {
        MTR_SCOPE("D3D12RenderCommandList", "set_render_targets");

        const D3D12Framebuffer& d3d12_framebuffer = static_cast<const D3D12Framebuffer&>(framebuffer);

        ENSURE(d3d12_framebuffer.rtv_handles.size() == render_target_accesses.size(),
               "Must have the same number of render targets and render target accesses");
        ENSURE(d3d12_framebuffer.dsv_handle.has_value() == depth_access.has_value(),
               "There must be either both a DSV handle and a depth target access, or neither");

        if(commands4) {
            // TODO: Decide if every framebuffer should be a separate renderpass

            if(in_render_pass) {
                commands4->EndRenderPass();
            }

            std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> render_target_descriptions;
            render_target_descriptions.reserve(render_target_accesses.size());

            for(uint32_t i = 0; i < render_target_accesses.size(); i++) {
                D3D12_RENDER_PASS_RENDER_TARGET_DESC desc{};
                desc.cpuDescriptor = d3d12_framebuffer.rtv_handles[i];
                desc.BeginningAccess = to_d3d12_beginning_access(render_target_accesses[i].begin);
                desc.EndingAccess = to_d3d12_ending_access(render_target_accesses[i].end);

                render_target_descriptions.emplace_back(desc);
            }

            if(depth_access) {
                D3D12_RENDER_PASS_DEPTH_STENCIL_DESC desc{};
                desc.cpuDescriptor = *d3d12_framebuffer.dsv_handle;
                desc.DepthBeginningAccess = to_d3d12_beginning_access(depth_access->begin);
                desc.DepthEndingAccess = to_d3d12_ending_access(depth_access->end);
                desc.StencilBeginningAccess = to_d3d12_beginning_access(depth_access->begin);
                desc.StencilEndingAccess = to_d3d12_ending_access(depth_access->end);

                commands4->BeginRenderPass(static_cast<UINT>(render_target_descriptions.size()),
                                           render_target_descriptions.data(),
                                           &desc,
                                           D3D12_RENDER_PASS_FLAG_NONE);

            } else {
                commands4->BeginRenderPass(static_cast<UINT>(render_target_descriptions.size()),
                                           render_target_descriptions.data(),
                                           nullptr,
                                           D3D12_RENDER_PASS_FLAG_NONE);
            }

            in_render_pass = true;

        } else {
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

            spdlog::warn("Render passes not available - falling back to something lame");
        }

        D3D12_VIEWPORT viewport{};
        viewport.MinDepth = 0;
        viewport.MaxDepth = 1;
        viewport.Width = d3d12_framebuffer.width;
        viewport.Height = d3d12_framebuffer.height;
        commands->RSSetViewports(1, &viewport);

        D3D12_RECT scissor_rect{};
        scissor_rect.right = static_cast<LONG>(d3d12_framebuffer.width);
        scissor_rect.bottom = static_cast<LONG>(d3d12_framebuffer.height);
        commands->RSSetScissorRects(1, &scissor_rect);
    }

    void D3D12RenderCommandList::set_pipeline_state(const RenderPipelineState& state) {
        MTR_SCOPE("D3D12RenderCommandList", "set_pipeline_state");

        const auto& d3d12_state = static_cast<const D3D12RenderPipelineState&>(state);

        if(current_render_pipeline_state == nullptr || current_render_pipeline_state->root_signature != d3d12_state.root_signature) {
            commands->SetGraphicsRootSignature(d3d12_state.root_signature.Get());
            is_render_material_bound = false;
        }

        commands->SetPipelineState(d3d12_state.pso.Get());

        current_render_pipeline_state = &d3d12_state;
    }

    void D3D12RenderCommandList::bind_render_resources(const BindGroup& bind_group) {
        MTR_SCOPE("D3D12RenderCommandList", "bind_render_resources");

        ENSURE(current_render_pipeline_state != nullptr, "Must bind a render pipeline before binding render resources");

        const auto& d3d12_bind_group = static_cast<const D3D12BindGroup&>(bind_group);
        for(const BoundResource<D3D12Buffer>& resource : d3d12_bind_group.used_buffers) {
            set_resource_state(*resource.resource, resource.states);
        }

        for(const BoundResource<D3D12Image>& resource : d3d12_bind_group.used_images) {
            set_resource_state(*resource.resource, resource.states);
        }

        if(current_descriptor_heap != d3d12_bind_group.heap) {
            commands->SetDescriptorHeaps(1, &d3d12_bind_group.heap);
            current_descriptor_heap = d3d12_bind_group.heap;
        }

        d3d12_bind_group.bind_to_graphics_signature(*commands.Get());

        is_render_material_bound = true;
    }

    void D3D12RenderCommandList::set_camera_idx(const uint32_t camera_idx) {
        MTR_SCOPE("D3D12RenderCommandList", "set_camera_idx");

        ENSURE(current_render_pipeline_state != nullptr, "Must bind a pipeline before setting the camera index");

        commands->SetGraphicsRoot32BitConstant(0, camera_idx, 0);
    }

    void D3D12RenderCommandList::bind_mesh_data(const MeshDataStore& mesh_data) {
        MTR_SCOPE("D3D12RenderCommandList", "bind_mesh_data");

        const auto& vertex_bindings = mesh_data.get_vertex_bindings();

        // If we have more than 16 vertex attributes, we probably have bigger problems
        std::array<D3D12_VERTEX_BUFFER_VIEW, 16> vertex_buffer_views;
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

        commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        is_mesh_data_bound = true;
    }

    void D3D12RenderCommandList::draw(const uint32_t num_indices, const uint32_t first_index, const uint32_t num_instances) {
        MTR_SCOPE("D3D12RenderCommandList", "draw");

        // ENSURE(is_render_material_bound, "Must bind material data to issue drawcalls");
        ENSURE(is_mesh_data_bound, "Must bind mesh data to issue drawcalls");
        ENSURE(current_render_pipeline_state != nullptr, "Must bind a render pipeline to issue drawcalls");

        commands->DrawIndexedInstanced(num_indices, num_instances, first_index, 0, 0);
    }

    ID3D12GraphicsCommandList4& D3D12RenderCommandList::get_commands4() {
        return *commands4.Get();
    }

    void D3D12RenderCommandList::prepare_for_submission() {
        MTR_SCOPE("D3D12RenderCommandList", "prepare_for_submission");
        if(in_render_pass && commands4) {
            commands4->EndRenderPass();
        }

        D3D12ComputeCommandList::prepare_for_submission();
    }
} // namespace rhi
