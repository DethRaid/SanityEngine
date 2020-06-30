#include "forward_pass.hpp"

#include <Tracy.hpp>
#include <TracyD3D12.hpp>
#include <entt/entity/registry.hpp>
#include <rx/core/log.h>

#include "core/types.hpp"
#include "loading/shader_loading.hpp"
#include "renderer/render_components.hpp"
#include "renderer/renderer.hpp"
#include "rhi/render_device.hpp"
#include "world/world.hpp"

namespace renderer {
    constexpr const char* SCENE_COLOR_RENDER_TARGET = "Scene color target";
    constexpr const char* SCENE_DEPTH_TARGET = "Scene depth target";

    RX_LOG("ForwardPass", logger);

    ForwardPass::ForwardPass(Renderer& renderer_in, const glm::uvec2& render_resolution) : renderer{&renderer_in} {
        ZoneScoped;
        auto& device = renderer_in.get_render_device();

        standard_pipeline = device.create_render_pipeline_state({
            .name = "Standard material pipeline",
            .vertex_shader = load_shader("standard.vertex"),
            .pixel_shader = load_shader("standard.pixel"),
            .render_target_formats = Rx::Array{ImageFormat::Rgba32F},
            .depth_stencil_format = ImageFormat::Depth32,
        });
        logger->verbose("Created standard pipeline");

        opaque_chunk_geometry_pipeline = device.create_render_pipeline_state({
            .name = "Opaque chunk geometry pipeline",
            .vertex_shader = load_shader("chunk.vertex"),
            .pixel_shader = load_shader("opaque_chunk.pixel"),
            .render_target_formats = Rx::Array{ImageFormat::Rgba32F},
            .depth_stencil_format = ImageFormat::Depth32,
        });
        logger->verbose("Created opaque chunk geometry pipeline");

        atmospheric_sky_pipeline = device.create_render_pipeline_state({
            .name = "Standard material pipeline",
            .vertex_shader = load_shader("fullscreen.vertex"),
            .pixel_shader = load_shader("atmospheric_sky.pixel"),
            .depth_stencil_state =
                {
                    .enable_depth_write = false,
                    .depth_func = CompareOp::Always,
                },
            .render_target_formats = Rx::Array{ImageFormat::Rgba32F},
            .depth_stencil_format = ImageFormat::Depth32,
        });
        logger->verbose("Created atmospheric pipeline");

        create_framebuffer(render_resolution);
    }

    ForwardPass::~ForwardPass() {
        ZoneScoped;
        // delete the scene framebuffer, atmospheric sky pipeline, and other resources we own

        auto& device = renderer->get_render_device();
    }

    void ForwardPass::render(ID3D12GraphicsCommandList4* commands, entt::registry& registry, const Uint32 frame_idx, const World& world) {
        ZoneScoped;

        TracyD3D12Zone(RenderDevice::tracy_context, commands, "ForwardPass::execute");
        PIXScopedEvent(commands, forward_pass_color, "ForwardPass::execute");

        begin_render_pass(commands);

        commands->SetGraphicsRootSignature(standard_pipeline->root_signature.get());

        const auto& bind_group = renderer->bind_global_resources_for_frame(frame_idx);
        bind_group->bind_to_graphics_signature(commands);

        // Hardcode camera 0 as the player camera
        // TODO: Decide if this is fine
        commands->SetGraphicsRoot32BitConstant(0, 0, RenderDevice::CAMERA_INDEX_ROOT_CONSTANT_OFFSET);

        // Draw atmosphere first because projection matrices are hard
        draw_atmosphere(commands, registry);

        draw_objects_in_scene(commands, registry, frame_idx);

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

        color_target_access = {.cpuDescriptor = device.create_rtv_handle(color_target),
                               .BeginningAccess = {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
                                                   .Clear = {.ClearValue = {.Format = DXGI_FORMAT_R32_FLOAT, .Color = {0, 0, 0, 0}}}},
                               .EndingAccess = {.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE}};

        depth_target_access = {.cpuDescriptor = device.create_dsv_handle(depth_target),
                               .DepthBeginningAccess = {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
                                                        .Clear = {.ClearValue = {.Format = DXGI_FORMAT_R32_FLOAT,
                                                                                 .DepthStencil = {.Depth = 1.0}}}},
                               .StencilBeginningAccess = {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD},
                               .DepthEndingAccess = {.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE},
                               .StencilEndingAccess = {.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD}};

        render_target_size = render_resolution;
    }

    TextureHandle ForwardPass::get_color_target_handle() const { return color_target_handle; }

    TextureHandle ForwardPass::get_depth_target_handle() const { return depth_target_handle; }

    void ForwardPass::begin_render_pass(ID3D12GraphicsCommandList4* commands) const {
        commands->BeginRenderPass(1, &color_target_access, &depth_target_access, D3D12_RENDER_PASS_FLAG_NONE);

        D3D12_VIEWPORT viewport{};
        viewport.MinDepth = 0;
        viewport.MaxDepth = 1;
        viewport.Width = render_target_size.x;
        viewport.Height = render_target_size.y;
        commands->RSSetViewports(1, &viewport);

        D3D12_RECT scissor_rect{};
        scissor_rect.right = static_cast<LONG>(render_target_size.x);
        scissor_rect.bottom = static_cast<LONG>(render_target_size.y);
        commands->RSSetScissorRects(1, &scissor_rect);
    }

    void ForwardPass::draw_objects_in_scene(ID3D12GraphicsCommandList4* commands, entt::registry& registry, const Uint32 frame_idx) {
        PIXScopedEvent(commands, forward_pass_color, "ForwardPass::draw_object_in_scene");

        commands->SetPipelineState(standard_pipeline->pso.get());

        {
            auto& model_matrix_buffer = renderer->get_model_matrix_for_frame(frame_idx);
            commands->SetGraphicsRootShaderResourceView(RenderDevice::MODEL_MATRIX_BUFFER_ROOT_PARAMETER_INDEX,
                                                        model_matrix_buffer.resource->GetGPUVirtualAddress());
        }

        const auto& mesh_storage = renderer->get_static_mesh_store();
        mesh_storage.bind_to_command_list(commands);

        auto& material_buffer = renderer->get_standard_material_buffer_for_frame(frame_idx);

        commands->SetGraphicsRootShaderResourceView(RenderDevice::MATERIAL_BUFFER_ROOT_PARAMETER_INDEX,
                                                    material_buffer.resource->GetGPUVirtualAddress());

        {
            const auto& renderable_view = registry.view<TransformComponent, StandardRenderableComponent>();
            renderable_view.each([&](const TransformComponent& transform, const StandardRenderableComponent& renderable) {
                // TODO: Frustum culling, view distance calculations, etc

                // TODO: Figure out the priority queues to put things in

                commands->SetGraphicsRoot32BitConstant(0, renderable.material.index, RenderDevice::MATERIAL_INDEX_ROOT_CONSTANT_OFFSET);

                const auto model_matrix_index = renderer->add_model_matrix_to_frame(transform, frame_idx);

                commands->SetGraphicsRoot32BitConstant(0, model_matrix_index, RenderDevice::MODEL_MATRIX_INDEX_ROOT_CONSTANT_OFFSET);

                commands->DrawIndexedInstanced(renderable.mesh.num_indices, 1, renderable.mesh.first_index, 0, 0);
            });
        }
    }

    void ForwardPass::draw_atmosphere(ID3D12GraphicsCommandList4* commands, entt::registry& registry) const {
        const auto atmosphere_view = registry.view<AtmosphericSkyComponent>();
        if(atmosphere_view.size() > 1) {
            logger->error("May only have one atmospheric sky component in a scene");

        } else {
            PIXScopedEvent(commands, forward_pass_color, "ForwardPass::draw_atmosphere");

            commands->SetPipelineState(atmospheric_sky_pipeline->pso.get());

            commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            commands->DrawInstanced(3, 1, 0, 0);
        }
    }

} // namespace renderer
