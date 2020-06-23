#include "ui_render_pass.hpp"

#include <Tracy.hpp>
#include <TracyD3D12.hpp>
#include <imgui/imgui.h>

#include "loading/shader_loading.hpp"
#include "renderer/renderer.hpp"
#include "rhi/render_device.hpp"
#include "rhi/resources.hpp"

namespace renderer {
    UiPass::UiPass(Renderer& renderer_in) : renderer{&renderer_in} {
        ZoneScoped;

        auto& device = renderer_in.get_render_device();

        const auto create_info = RenderPipelineStateCreateInfo{
            .name = "UI Pipeline",
            .vertex_shader = load_shader("ui.vertex"),
            .pixel_shader = load_shader("ui.pixel"),

            .input_assembler_layout = InputAssemblerLayout::DearImGui,

            .blend_state = BlendState{.render_target_blends = {RenderTargetBlendState{.enabled = true}}},

            .rasterizer_state =
                RasterizerState{
                    .cull_mode = CullMode::None,
                },

            .depth_stencil_state =
                DepthStencilState{
                    .enable_depth_test = false,
                    .enable_depth_write = false,
                },

            .render_target_formats = Rx::Array{ImageFormat::Rgba8},
        };

        ui_pipeline = device.create_render_pipeline_state(create_info);
    }

    void UiPass::render(ID3D12GraphicsCommandList4* commands,
                              entt::registry& /* registry */,
                              Uint32 /* frame_idx */,
                              const World& /* world */) {
        ZoneScoped;

        auto& device = renderer->get_render_device();

        TracyD3D12Zone(RenderDevice::tracy_context, commands, "UiRenderPass::execute");
        PIXScopedEvent(commands, PIX_COLOR_DEFAULT, "UiRenderPass::execute");
        {
            const auto* backbuffer_framebuffer = device.get_backbuffer_framebuffer();

            const auto
                backbuffer_access = D3D12_RENDER_PASS_RENDER_TARGET_DESC{.cpuDescriptor = backbuffer_framebuffer->rtv_handles[0],
                                                                         .BeginningAccess =
                                                                             {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE},
                                                                         .EndingAccess = {
                                                                             .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE}};

            commands->BeginRenderPass(1, &backbuffer_access, nullptr, D3D12_RENDER_PASS_FLAG_NONE);
        }

        // TODO: Instead of allocating and destroying buffers every frame, make a couple large buffers for the UI mesh data to live in

        ImDrawData* draw_data = ImGui::GetDrawData();
        if(draw_data == nullptr) {
            commands->EndRenderPass();
            return;
        }

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
                                                                  .SizeInBytes = vertex_buffer.size,
                                                                  .StrideInBytes = sizeof(ImDrawVert)};
                    commands->IASetVertexBuffers(0, 1, &vb_view);

                    const auto ib_view = D3D12_INDEX_BUFFER_VIEW{.BufferLocation = index_buffer.resource->GetGPUVirtualAddress(),
                                                                 .SizeInBytes = index_buffer.size,
                                                                 .Format = DXGI_FORMAT_R32_UINT};
                    commands->IASetIndexBuffer(&ib_view);

                    commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                }

                for(const auto& cmd : cmd_list->CmdBuffer) {
                    if(cmd.UserCallback) {
                        cmd.UserCallback(cmd_list, &cmd);

                    } else {
                        const auto imgui_material_idx = reinterpret_cast<uint64_t>(cmd.TextureId);
                        const auto texture_idx = static_cast<Uint32>(imgui_material_idx);
                        commands->SetGraphicsRoot32BitConstant(0, texture_idx, 1);

                        {
                            const auto& clip_rect = cmd.ClipRect;
                            const auto pos = draw_data->DisplayPos;
                            const auto top_left_x = clip_rect.x - pos.x;
                            const auto top_left_y = clip_rect.y - pos.y;
                            const auto width = clip_rect.z - pos.x;
                            const auto height = clip_rect.w - pos.y;
                            const auto rect = D3D12_RECT{.left = static_cast<LONG>(top_left_x),
                                                         .top = static_cast<LONG>(top_left_y),
                                                         .right = static_cast<LONG>(top_left_x + width),
                                                         .bottom = static_cast<LONG>(top_left_y + height)};
                            commands->RSSetScissorRects(1, &rect);
                        }

                        commands->DrawIndexedInstanced(cmd.ElemCount, 1, cmd.IdxOffset, 0, 0);
                    }
                }

                {
                    ZoneScopedN("Free vertex and index buffers");
                    device.return_staging_buffer(std::move(vertex_buffer));
                    device.return_staging_buffer(std::move(index_buffer));
                }
            }
        }
        commands->EndRenderPass();
    }
} // namespace renderer
