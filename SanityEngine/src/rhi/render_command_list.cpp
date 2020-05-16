#include "render_command_list.hpp"

#include "../core/ensure.hpp"
#include "bind_group.hpp"
#include "d3dx12.hpp"
#include "framebuffer.hpp"
#include "helpers.hpp"
#include "imgui/imgui.h"
#include "mesh_data_store.hpp"
#include "render_pipeline_state.hpp"

using std::move;

namespace rhi {
    RenderCommandList::RenderCommandList(ComPtr<ID3D12GraphicsCommandList4> cmds, RenderDevice& device_in)
        : ComputeCommandList{move(cmds), device_in} {}

    RenderCommandList::RenderCommandList(RenderCommandList&& old) noexcept
        : ComputeCommandList{move(old)},
          in_render_pass{old.in_render_pass},
          current_render_pipeline_state{old.current_render_pipeline_state},
          is_render_material_bound{old.is_render_material_bound} {}

    RenderCommandList& RenderCommandList::operator=(RenderCommandList&& old) noexcept {
        in_render_pass = old.in_render_pass;
        current_render_pipeline_state = old.current_render_pipeline_state;
        is_render_material_bound = old.is_render_material_bound;

        return static_cast<RenderCommandList&>(ComputeCommandList::operator=(move(old)));
    }

    void RenderCommandList::bind_framebuffer(const Framebuffer& framebuffer,
                                            std::vector<RenderTargetAccess> render_target_accesses,
                                            std::optional<RenderTargetAccess> depth_access) {
        ENSURE(framebuffer.rtv_handles.size() == render_target_accesses.size(),
               "Must have the same number of render targets and render target accesses");
        ENSURE(framebuffer.dsv_handle.has_value() == depth_access.has_value(),
               "There must be either both a DSV handle and a depth target access, or neither");

        // TODO: Decide if every framebuffer should be a separate renderpass

        if(in_render_pass) {
            commands->EndRenderPass();
        }

        std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> render_target_descriptions;
        render_target_descriptions.reserve(render_target_accesses.size());

        for(uint32_t i = 0; i < render_target_accesses.size(); i++) {
            D3D12_RENDER_PASS_RENDER_TARGET_DESC desc{};
            desc.cpuDescriptor = framebuffer.rtv_handles[i];
            desc.BeginningAccess = to_d3d12_beginning_access(render_target_accesses[i].begin);
            desc.EndingAccess = to_d3d12_ending_access(render_target_accesses[i].end);

            render_target_descriptions.emplace_back(desc);
        }

        if(depth_access) {
            D3D12_RENDER_PASS_DEPTH_STENCIL_DESC desc{};
            desc.cpuDescriptor = *framebuffer.dsv_handle;
            desc.DepthBeginningAccess = to_d3d12_beginning_access(depth_access->begin);
            desc.DepthEndingAccess = to_d3d12_ending_access(depth_access->end);
            desc.StencilBeginningAccess = to_d3d12_beginning_access(depth_access->begin);
            desc.StencilEndingAccess = to_d3d12_ending_access(depth_access->end);

            commands->BeginRenderPass(static_cast<UINT>(render_target_descriptions.size()),
                                      render_target_descriptions.data(),
                                      &desc,
                                      D3D12_RENDER_PASS_FLAG_NONE);

        } else {
            commands->BeginRenderPass(static_cast<UINT>(render_target_descriptions.size()),
                                      render_target_descriptions.data(),
                                      nullptr,
                                      D3D12_RENDER_PASS_FLAG_NONE);
        }

        in_render_pass = true;

        D3D12_VIEWPORT viewport{};
        viewport.MinDepth = 0;
        viewport.MaxDepth = 1;
        viewport.Width = framebuffer.width;
        viewport.Height = framebuffer.height;
        commands->RSSetViewports(1, &viewport);

        D3D12_RECT scissor_rect{};
        scissor_rect.right = static_cast<LONG>(framebuffer.width);
        scissor_rect.bottom = static_cast<LONG>(framebuffer.height);
        commands->RSSetScissorRects(1, &scissor_rect);
    }

    void RenderCommandList::bind_ui_mesh(const Buffer& vertex_buffer, const Buffer& index_buffer) {
        set_resource_state(vertex_buffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        set_resource_state(index_buffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);

        const auto vb_view = D3D12_VERTEX_BUFFER_VIEW{.BufferLocation = vertex_buffer.resource->GetGPUVirtualAddress(),
                                                      .SizeInBytes = vertex_buffer.size,
                                                      .StrideInBytes = sizeof(ImDrawVert)};
        commands->IASetVertexBuffers(0, 1, &vb_view);

        const auto ib_view = D3D12_INDEX_BUFFER_VIEW{.BufferLocation = index_buffer.resource->GetGPUVirtualAddress(),
                                                     .SizeInBytes = index_buffer.size,
                                                     .Format = DXGI_FORMAT_R32_UINT};
        commands->IASetIndexBuffer(&ib_view);

        commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        is_mesh_data_bound = true;
    }

    void RenderCommandList::set_pipeline_state(const RenderPipelineState& state) {
        if(current_render_pipeline_state == nullptr || current_render_pipeline_state->root_signature != state.root_signature) {
            commands->SetGraphicsRootSignature(state.root_signature.Get());
            is_render_material_bound = false;
        }

        commands->SetPipelineState(state.pso.Get());

        current_render_pipeline_state = &state;
    }

    void RenderCommandList::set_viewport(const glm::vec2& display_pos, const glm::vec2& display_size) const {
        const auto viewport = D3D12_VIEWPORT{.TopLeftX = display_pos.x,
                                             .TopLeftY = display_pos.y,
                                             .Width = display_size.x,
                                             .Height = display_size.y,
                                             .MinDepth = 0,
                                             .MaxDepth = 1};
        commands->RSSetViewports(1, &viewport);
    }

    void RenderCommandList::set_scissor_rect(const glm::uvec2& pos, const glm::uvec2& size) const {
        const auto rect = D3D12_RECT{.left = static_cast<LONG>(pos.x),
                                     .top = static_cast<LONG>(pos.y),
                                     .right = static_cast<LONG>(pos.x + size.x),
                                     .bottom = static_cast<LONG>(pos.y + size.y)};
        commands->RSSetScissorRects(1, &rect);
    }

    void RenderCommandList::bind_render_resources(const BindGroup& bind_group) {
        ENSURE(current_render_pipeline_state != nullptr, "Must bind a render pipeline before binding render resources");

        for(const BoundResource<Buffer>& resource : bind_group.used_buffers) {
            set_resource_state(*resource.resource, resource.states);
        }

        for(const BoundResource<Image>& resource : bind_group.used_images) {
            set_resource_state(*resource.resource, resource.states);
        }

        if(current_descriptor_heap != bind_group.heap) {
            commands->SetDescriptorHeaps(1, &bind_group.heap);
            current_descriptor_heap = bind_group.heap;
        }

        bind_group.bind_to_graphics_signature(*commands.Get());

        is_render_material_bound = true;
    }

    void RenderCommandList::set_camera_idx(const uint32_t camera_idx) const {
        ENSURE(current_render_pipeline_state != nullptr, "Must bind a pipeline before setting the camera index");

        commands->SetGraphicsRoot32BitConstant(0, camera_idx, 0);
    }

    void RenderCommandList::set_material_idx(const uint32_t idx) const {
        ENSURE(current_render_pipeline_state != nullptr, "Must bind a pipeline before setting the material index");

        commands->SetGraphicsRoot32BitConstant(0, idx, 1);
    }

    void RenderCommandList::draw(const uint32_t num_indices, const uint32_t first_index, const uint32_t num_instances) const {
        // ENSURE(is_render_material_bound, "Must bind material data to issue drawcalls");
        ENSURE(is_mesh_data_bound, "Must bind mesh data to issue drawcalls");
        ENSURE(current_render_pipeline_state != nullptr, "Must bind a render pipeline to issue drawcalls");

        commands->DrawIndexedInstanced(num_indices, num_instances, first_index, 0, 0);
    }

    void RenderCommandList::prepare_for_submission() {
        if(in_render_pass) {
            commands->EndRenderPass();
        }

        ComputeCommandList::prepare_for_submission();
    }
} // namespace rhi
