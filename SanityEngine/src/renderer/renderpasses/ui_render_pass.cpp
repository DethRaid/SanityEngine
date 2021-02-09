#include "ui_render_pass.hpp"

#include <rx/core/log.h>


#include "Tracy.hpp"
#include "TracyD3D12.hpp"
#include "imgui/imgui.h"
#include "loading/shader_loading.hpp"
#include "renderer/debugging/pix.hpp"
#include "renderer/renderer.hpp"
#include "renderer/rhi/render_device.hpp"
#include "renderer/rhi/resources.hpp"

namespace sanity::engine::renderer {
    RX_LOG("DearImGuiRenderPass", logger);
	
    DearImGuiRenderPass::DearImGuiRenderPass(Renderer& renderer_in) : renderer{&renderer_in} {
        ZoneScoped;

        const auto blend_state = BlendState{.render_target_blends = {RenderTargetBlendState{.enabled = true},
                                                                     RenderTargetBlendState{.enabled = false},
                                                                     RenderTargetBlendState{.enabled = false},
                                                                     RenderTargetBlendState{.enabled = false},
                                                                     RenderTargetBlendState{.enabled = false},
                                                                     RenderTargetBlendState{.enabled = false},
                                                                     RenderTargetBlendState{.enabled = false},
                                                                     RenderTargetBlendState{.enabled = false}}};

        auto& device = renderer_in.get_render_backend();
        ui_pipeline = device.create_render_pipeline_state({
            .name = "UI Pipeline",
            .vertex_shader = load_shader("ui.vertex"),
            .pixel_shader = load_shader("ui.pixel"),

            .input_assembler_layout = InputAssemblerLayout::DearImGui,

            .blend_state = blend_state,

            .rasterizer_state =
                {
                    .cull_mode = CullMode::None,
                },

            .depth_stencil_state =
                {
                    .enable_depth_test = false,
                    .enable_depth_write = false,
                },

            .render_target_formats = Rx::Array{TextureFormat::Rgba8},
        });
    }

    void DearImGuiRenderPass::render(ID3D12GraphicsCommandList4* commands,
                        entt::registry& /* registry */,
                        Uint32 /* frame_idx */) {
        ZoneScoped;

        ImDrawData* draw_data = ImGui::GetDrawData();
        if(draw_data == nullptr) {
            // Nothing to draw? Don't draw it
            return;
        }

        auto& device = renderer->get_render_backend();

        TracyD3D12Zone(RenderBackend::tracy_context, commands, "UiRenderPass::render");
        PIXScopedEvent(commands, PIX_COLOR_DEFAULT, "UiRenderPass::render");
        {
            const auto backbuffer_rtv_handle = device.get_backbuffer_rtv_handle();

            const auto
                backbuffer_access = D3D12_RENDER_PASS_RENDER_TARGET_DESC{.cpuDescriptor = backbuffer_rtv_handle,
                                                                         .BeginningAccess =
                                                                             {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE},
                                                                         .EndingAccess = {
                                                                             .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE}};

            commands->BeginRenderPass(1, &backbuffer_access, nullptr, D3D12_RENDER_PASS_FLAG_NONE);
        }

        // TODO: Instead of allocating and destroying buffers every frame, make a couple large buffers for the UI mesh data to live in

        commands->SetPipelineState(ui_pipeline->pso.Get());

        {
            const auto viewport = D3D12_VIEWPORT{.TopLeftX = draw_data->DisplayPos.x,
                                                 .TopLeftY = draw_data->DisplayPos.y,
                                                 .Width = draw_data->DisplaySize.x,
                                                 .Height = draw_data->DisplaySize.y,
                                                 .MinDepth = 0,
                                                 .MaxDepth = 1};
            commands->RSSetViewports(1, &viewport);
        }

        {
            ZoneScopedN("Issue UI drawcalls");
            for(int32_t i = 0; i < draw_data->CmdListsCount; i++) {
                PIXScopedEvent(commands, PIX_COLOR_DEFAULT, "Renderer::render_ui::draw_imgui_list(%d)", i);
                const auto* cmd_list = draw_data->CmdLists[i];
                const auto* imgui_vertices = cmd_list->VtxBuffer.Data;
                const auto* imgui_indices = cmd_list->IdxBuffer.Data;

                const auto vertex_buffer_size = static_cast<Uint32>(cmd_list->VtxBuffer.size_in_bytes());
                const auto index_buffer_size = static_cast<Uint32>(cmd_list->IdxBuffer.size_in_bytes());

                auto vertex_buffer = device.get_staging_buffer(vertex_buffer_size);
                memcpy(vertex_buffer.mapped_ptr, imgui_vertices, vertex_buffer_size);

                auto index_buffer = device.get_staging_buffer(index_buffer_size);
                memcpy(index_buffer.mapped_ptr, imgui_indices, index_buffer_size);

                {
                    const auto vb_view = D3D12_VERTEX_BUFFER_VIEW{.BufferLocation = vertex_buffer.resource->GetGPUVirtualAddress(),
                                                                  .SizeInBytes = static_cast<Uint32>(vertex_buffer.size),
                                                                  .StrideInBytes = sizeof(ImDrawVert)};
                    commands->IASetVertexBuffers(0, 1, &vb_view);

                    const auto index_buffer_format = sizeof(ImDrawIdx) == sizeof(unsigned int) ? DXGI_FORMAT_R32_UINT :
                                                                                                 DXGI_FORMAT_R16_UINT;
                	
                    const auto ib_view = D3D12_INDEX_BUFFER_VIEW{.BufferLocation = index_buffer.resource->GetGPUVirtualAddress(),
                                                                 .SizeInBytes = static_cast<Uint32>(index_buffer.size),
                                                                 .Format = index_buffer_format};
                    commands->IASetIndexBuffer(&ib_view);

                    commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                }

                for(const auto& cmd : cmd_list->CmdBuffer) {
                    if(cmd.UserCallback) {
                        cmd.UserCallback(cmd_list, &cmd);

                    } else {
                        const auto imgui_material_idx = reinterpret_cast<uint64_t>(cmd.TextureId);
                        const auto material_idx = static_cast<Uint32>(imgui_material_idx);
                        commands->SetGraphicsRoot32BitConstant(0, material_idx, RenderBackend::MATERIAL_INDEX_ROOT_CONSTANT_OFFSET);

                        {
                            const auto& clip_rect = cmd.ClipRect;
                            const auto pos = draw_data->DisplayPos;
                            const auto top_left_x = clip_rect.x - pos.x;
                            const auto top_left_y = clip_rect.y - pos.y;
                            const auto bottom_right_x = clip_rect.z - pos.x;
                            const auto bottom_right_y = clip_rect.w - pos.y;
                            const auto rect = D3D12_RECT{.left = static_cast<LONG>(top_left_x),
                                                         .top = static_cast<LONG>(top_left_y),
                                                         .right = static_cast<LONG>(bottom_right_x),
                                                         .bottom = static_cast<LONG>(bottom_right_y)};
                            commands->RSSetScissorRects(1, &rect);
                        }

                        commands->DrawIndexedInstanced(cmd.ElemCount, 1, cmd.IdxOffset, 0, 0);
                    }
                }

                {
                    ZoneScopedN("Free vertex and index buffers");
                    device.return_staging_buffer(Rx::Utility::move(vertex_buffer));
                    device.return_staging_buffer(Rx::Utility::move(index_buffer));
                }
            }
        }
        commands->EndRenderPass();
    }
} // namespace renderer
