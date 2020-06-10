#include "forward_pass.hpp"

#include <vector>

#include <entt/entity/registry.hpp>
#include <minitrace.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "../render_components.hpp"

namespace renderer {
    std::shared_ptr<spdlog::logger> ForwardPass::logger{spdlog::stdout_color_st("ForwardPass")};

    ForwardPass::~ForwardPass() {
        // delete the scene framebuffer, atmospheric sky pipeline, and other resources we own
    }

    void ForwardPass::execute(ID3D12GraphicsCommandList4* commands, entt::registry& registry) {
        begin_render_pass(commands);

        draw_objects_in_scene(commands, registry, material_bind_group);

        draw_sky(commands, registry);

        commands->EndRenderPass();
    }

    void ForwardPass::begin_render_pass(ID3D12GraphicsCommandList4* commands) const {
        const auto render_target_accesses = std::vector{
            // Scene color
            D3D12_RENDER_PASS_RENDER_TARGET_DESC{.cpuDescriptor = scene_framebuffer->rtv_handles[0],
                                                 .BeginningAccess = {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
                                                                     .Clear = {.ClearValue = {.Format = DXGI_FORMAT_R32_FLOAT,
                                                                                              .Color = {0, 0, 0, 0}}}},
                                                 .EndingAccess = {.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE}}};

        const auto
            depth_access = D3D12_RENDER_PASS_DEPTH_STENCIL_DESC{.cpuDescriptor = *scene_framebuffer->dsv_handle,
                                                                .DepthBeginningAccess =
                                                                    {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
                                                                     .Clear = {.ClearValue = {.Format = DXGI_FORMAT_R32_FLOAT,
                                                                                              .DepthStencil = {.Depth = 1.0}}}},
                                                                .StencilBeginningAccess = {D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD},
                                                                .DepthEndingAccess = {D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE},
                                                                .StencilEndingAccess = {D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD}};

        commands->BeginRenderPass(static_cast<UINT>(render_target_accesses.size()),
                                  render_target_accesses.data(),
                                  &depth_access,
                                  D3D12_RENDER_PASS_FLAG_NONE);

        D3D12_VIEWPORT viewport{};
        viewport.MinDepth = 0;
        viewport.MaxDepth = 1;
        viewport.Width = scene_framebuffer->width;
        viewport.Height = scene_framebuffer->height;
        commands->RSSetViewports(1, &viewport);

        D3D12_RECT scissor_rect{};
        scissor_rect.right = static_cast<LONG>(scene_framebuffer->width);
        scissor_rect.bottom = static_cast<LONG>(scene_framebuffer->height);
        commands->RSSetScissorRects(1, &scissor_rect);
    }

    void ForwardPass::draw_objects_in_scene(ID3D12GraphicsCommandList4* commands,
                                            entt::registry& registry,
                                            const rhi::BindGroup& material_bind_group) {
        commands->SetGraphicsRootSignature(standard_pipeline->root_signature.Get());
        commands->SetPipelineState(standard_pipeline->pso.Get());

        // Hardcode camera 0 as the player camera
        // TODO: Decide if this is fine
        commands->SetGraphicsRoot32BitConstant(0, 0, 0);

        material_bind_group.bind_to_graphics_signature(commands);
        static_mesh_storage->bind_to_command_list(commands);

        {
            MTR_SCOPE("Renderer", "Render static meshes");
            const auto& renderable_view = registry.view<StandardRenderableComponent>();
            std::vector<StandardMaterial> materials_for_draw;
            materials_for_draw.reserve(renderable_view.size());

            renderable_view.each([&](const StandardRenderableComponent& mesh_renderable) {
                const auto material_idx = static_cast<uint32_t>(materials_for_draw.size());
                materials_for_draw.push_back(mesh_renderable.material);

                commands->SetGraphicsRoot32BitConstant(0, material_idx, 1);
                commands->DrawIndexedInstanced(mesh_renderable.mesh.num_indices, 1, mesh_renderable.mesh.first_index, 0, 0);
            });
        }
    }

    void ForwardPass::draw_sky(ID3D12GraphicsCommandList4* command_list, entt::registry& registry) const {
        const auto atmosphere_view = registry.view<AtmosphericSkyComponent>();
        if(atmosphere_view.size() > 1) {
            logger->error("May only have one atmospheric sky component in a scene");

        } else {
            command_list->SetPipelineState(atmospheric_sky_pipeline->pso.Get());
            command_list->DrawInstanced(3, 1, 0, 0);
        }
    }

} // namespace renderer
