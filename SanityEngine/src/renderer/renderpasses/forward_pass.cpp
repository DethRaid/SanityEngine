#include "forward_pass.hpp"

#include <Tracy.hpp>
#include <entt/entity/registry.hpp>
#include <rx/core/log.h>

#include "loading/shader_loading.hpp"
#include "renderer/render_components.hpp"
#include "renderer/renderer.hpp"
#include "rhi/render_device.hpp"

namespace renderer {
    constexpr const char* SCENE_COLOR_RENDER_TARGET = "Scene color target";
    constexpr const char* SCENE_DEPTH_TARGET = "Scene depth target";

    RX_LOG("ForwardPass", logger);

    ForwardPass::ForwardPass(Renderer& renderer_in, const glm::uvec2& render_resolution) : renderer{&renderer_in} {
        auto& device = renderer_in.get_render_device();

        {
            const auto standard_pipeline_create_info = RenderPipelineStateCreateInfo{
                .name = "Standard material pipeline",
                .vertex_shader = load_shader("standard.vertex"),
                .pixel_shader = load_shader("standard.pixel"),
                .render_target_formats = Rx::Array{ImageFormat::Rgba32F},
                .depth_stencil_format = ImageFormat::Depth32,
            };

            standard_pipeline = device.create_render_pipeline_state(standard_pipeline_create_info);

            logger->verbose("Created standard pipeline");
        }

        {
            const auto atmospheric_sky_create_info = RenderPipelineStateCreateInfo{
                .name = "Atmospheric Sky",
                .vertex_shader = load_shader("fullscreen.vertex"),
                .pixel_shader = load_shader("atmospheric_sky.pixel"),
                .depth_stencil_state = {.enable_depth_test = true, .enable_depth_write = false, .depth_func = CompareOp::LessOrEqual},
                .render_target_formats = Rx::Array{ImageFormat::Rgba32F},
                .depth_stencil_format = ImageFormat::Depth32,
            };

            atmospheric_sky_pipeline = device.create_render_pipeline_state(atmospheric_sky_create_info);
        }

        create_framebuffer(render_resolution);
    }

    ForwardPass::~ForwardPass() {
        // delete the scene framebuffer, atmospheric sky pipeline, and other resources we own
    }

    void ForwardPass::execute(ID3D12GraphicsCommandList4* commands, entt::registry& registry, const Uint32 frame_idx) {
        ZoneScoped;
        TracyD3D12Zone(RenderDevice::tracy_context, commands, "ForwardPass");

        begin_render_pass(commands);

        const auto& bind_group = renderer->bind_global_resources_for_frame(frame_idx);

        draw_objects_in_scene(commands, registry, *bind_group, frame_idx);

        draw_chunks(commands, registry, frame_idx);

        draw_atmosphere(commands, registry);

        commands->EndRenderPass();
    }

    void ForwardPass::create_framebuffer(const glm::uvec2& render_resolution) {
        auto& device = renderer->get_render_device();

        const auto color_target_create_info = ImageCreateInfo{
            .name = SCENE_COLOR_RENDER_TARGET,
            .usage = ImageUsage::RenderTarget,
            .format = ImageFormat::Rgba32F,
            .width = render_resolution.x,
            .height = render_resolution.y,
            .enable_resource_sharing = true,
        };

        color_target_handle = renderer->create_image(color_target_create_info);

        const auto depth_target_create_info = ImageCreateInfo{
            .name = SCENE_DEPTH_TARGET,
            .usage = ImageUsage::DepthStencil,
            .format = ImageFormat::Depth32,
            .width = render_resolution.x,
            .height = render_resolution.y,
        };

        depth_target_handle = renderer->create_image(depth_target_create_info);

        const auto& color_target = renderer->get_image(color_target_handle);
        const auto& depth_target = renderer->get_image(depth_target_handle);

        scene_framebuffer = device.create_framebuffer(Rx::Array{&color_target}, &depth_target);
    }

    TextureHandle ForwardPass::get_color_target_handle() const { return color_target_handle; }

    TextureHandle ForwardPass::get_depth_target_handle() const { return depth_target_handle; }

    void ForwardPass::begin_render_pass(ID3D12GraphicsCommandList4* commands) const {
        ZoneScoped;
        const auto render_target_accesses = Rx::Array{
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
                                            const BindGroup& material_bind_group,
                                            const Uint32 frame_idx) {
        ZoneScoped;
        commands->SetGraphicsRootSignature(standard_pipeline->root_signature.Get());
        commands->SetPipelineState(standard_pipeline->pso.Get());

        // Hardcode camera 0 as the player camera
        // TODO: Decide if this is fine
        commands->SetGraphicsRoot32BitConstant(0, 0, 0);

        material_bind_group.bind_to_graphics_signature(commands);

        const auto& mesh_storage = renderer->get_static_mesh_store();
        mesh_storage.bind_to_command_list(commands);

        auto& material_buffer = renderer->get_standard_material_buffer_for_frame(frame_idx);

        commands->SetGraphicsRootShaderResourceView(RenderDevice::MATERIAL_BUFFER_ROOT_PARAMETER_INDEX,
                                                    material_buffer.resource->GetGPUVirtualAddress());

        {
            ZoneScopedN("Render static meshes");
            const auto& renderable_view = registry.view<StandardRenderableComponent>();
            renderable_view.each([&](const StandardRenderableComponent& mesh_renderable) {
                commands->SetGraphicsRoot32BitConstant(0,
                                                       mesh_renderable.material.index,
                                                       RenderDevice::MATERIAL_INDEX_ROOT_CONSTANT_OFFSET);
                commands->DrawIndexedInstanced(mesh_renderable.mesh.num_indices, 1, mesh_renderable.mesh.first_index, 0, 0);
            });
        }
    }

    void ForwardPass::draw_chunks(ID3D12GraphicsCommandList4* commands, entt::registry& registry, const Uint32 frame_idx) {
        if(opaque_chunk_geometry_pipeline) {
            commands->SetPipelineState(opaque_chunk_geometry_pipeline->pso.Get());
        }

        // TODO: Bind the opaque chunk geometry material

        registry.view<ChunkMeshComponent>().each([&](const ChunkMeshComponent& chunk_mesh) {
            commands->DrawIndexedInstanced(chunk_mesh.mesh.num_indices, 1, chunk_mesh.mesh.first_index, 0, 0);
        });
    }

    void ForwardPass::draw_atmosphere(ID3D12GraphicsCommandList4* commands, entt::registry& registry) const {
        ZoneScoped;
        const auto atmosphere_view = registry.view<AtmosphericSkyComponent>();
        if(atmosphere_view.size() > 1) {
            logger->error("May only have one atmospheric sky component in a scene");

        } else {
            commands->SetPipelineState(atmospheric_sky_pipeline->pso.Get());
            commands->DrawInstanced(3, 1, 0, 0);
        }
    }

} // namespace renderer
