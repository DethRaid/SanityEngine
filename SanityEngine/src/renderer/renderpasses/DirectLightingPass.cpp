#include "DirectLightingPass.hpp"

#include "Tracy.hpp"
#include "TracyD3D12.hpp"
#include "entt/entity/registry.hpp"
#include "loading/shader_loading.hpp"
#include "renderer/render_components.hpp"
#include "renderer/renderer.hpp"
#include "renderer/rhi/render_backend.hpp"
#include "rx/core/log.h"
#include "world/world.hpp"

namespace sanity::engine::renderer {
    constexpr const char* SCENE_COLOR_RENDER_TARGET = "Scene color target";
    constexpr const char* OBJECT_ID_TARGET = "Object ID";
    constexpr const char* SCENE_DEPTH_TARGET = "Scene depth target";

    RX_LOG("ObjectsPass", logger);

    DirectLightingPass::DirectLightingPass(Renderer& renderer_in, const glm::uvec2& render_resolution) : renderer{&renderer_in} {
        ZoneScoped;
        auto& device = renderer_in.get_render_backend();

        standard_pipeline = device.create_render_pipeline_state({
            .name = "Standard material pipeline",
            .vertex_shader = load_shader("standard.vertex"),
            .pixel_shader = load_shader("standard.pixel"),
            .render_target_formats = Rx::Array{TextureFormat::Rgba16F, TextureFormat::R32UInt},
            .depth_stencil_format = TextureFormat::Depth32,
        });
        logger->verbose("Created standard pipeline");

        outline_pipeline = device.create_render_pipeline_state({
            .name = "Standard material pipeline",
            .vertex_shader = load_shader("standard.vertex"),
            .pixel_shader = load_shader("standard.pixel"),
            .rasterizer_state = RasterizerState{.cull_mode = CullMode::Front},
            .render_target_formats = Rx::Array{TextureFormat::Rgba16F, TextureFormat::R32UInt},
            .depth_stencil_format = TextureFormat::Depth32,
        });
        logger->verbose("Created standard pipeline");

        atmospheric_sky_pipeline = device.create_render_pipeline_state({
            .name = "Standard material pipeline",
            .vertex_shader = load_shader("fullscreen.vertex"),
            .pixel_shader = load_shader("atmospheric_sky.pixel"),
            .depth_stencil_state =
                {
                    .enable_depth_write = false,
                    .depth_func = CompareOp::Always,
                },
            .render_target_formats = Rx::Array{TextureFormat::Rgba16F, TextureFormat::R32UInt},
            .depth_stencil_format = TextureFormat::Depth32,
        });
        logger->verbose("Created atmospheric pipeline");

        create_framebuffer(render_resolution);

        set_resource_usage(color_target_handle, D3D12_RESOURCE_STATE_RENDER_TARGET);
        set_resource_usage(object_id_target_handle, D3D12_RESOURCE_STATE_RENDER_TARGET);
        set_resource_usage(depth_target_handle, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }

    DirectLightingPass::~DirectLightingPass() {
        ZoneScoped;
        // delete the scene framebuffer, atmospheric sky pipeline, and other resources we own

        auto& device = renderer->get_render_backend();
    }

    void DirectLightingPass::record_work(ID3D12GraphicsCommandList4* commands, entt::registry& registry, const Uint32 frame_idx) {
        ZoneScoped;

        TracyD3D12Zone(RenderBackend::tracy_render_context, commands, "ObjectsPass::render");
        PIXScopedEvent(commands, forward_pass_color, "ObjectsPass::render");

        begin_render_pass(commands);

        commands->SetGraphicsRootSignature(standard_pipeline->root_signature);

        // TODO: Bind global resources at the beginning of the frame, after everything is converted to The Root Signature
        auto* heap = renderer->get_render_backend().get_cbv_srv_uav_heap();
        commands->SetDescriptorHeaps(1, &heap);

        commands->SetGraphicsRoot32BitConstant(RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                               renderer->get_frame_constants_buffer(frame_idx).index,
                                               RenderBackend::FRAME_CONSTANTS_BUFFER_INDEX_ROOT_CONSTANT_OFFSET);

        // Hardcode camera 0 as the player camera
        // TODO: Make a camera handle system
        // More important TODO: Generalize this whole "allocate slots from an array" system I seem to be relying on
        commands->SetGraphicsRoot32BitConstant(RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                               0,
                                               RenderBackend::CAMERA_INDEX_ROOT_CONSTANT_OFFSET);

        const auto& material_buffer = renderer->get_standard_material_buffer_for_frame(frame_idx);
        commands->SetGraphicsRoot32BitConstant(RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                               material_buffer.index,
                                               RenderBackend::DATA_BUFFER_INDEX_ROOT_PARAMETER_OFFSET);

        const auto& model_matrix_buffer = renderer->get_model_matrix_for_frame(frame_idx);
        commands->SetGraphicsRoot32BitConstant(RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                               model_matrix_buffer.index,
                                               RenderBackend::MODEL_MATRIX_BUFFER_INDEX_ROOT_CONSTANT_OFFSET);

        const auto& rt_scene = renderer->get_raytracing_scene();
        if(rt_scene.buffer.is_valid()) {
            const auto& rt_buffer = renderer->get_buffer(rt_scene.buffer);
            commands->SetGraphicsRootShaderResourceView(RenderBackend::RAYTRACING_SCENE_ROOT_PARAMETER_INDEX,
                                                        rt_buffer->resource->GetGPUVirtualAddress());
        }
    	
        commands->SetGraphicsRootDescriptorTable(RenderBackend::RESOURCES_ARRAY_ROOT_PARAMETER_INDEX,
                                                 renderer->get_resource_array_gpu_descriptor(frame_idx));

        // Draw atmosphere first because projection matrices are hard
        draw_atmosphere(commands, registry);

        draw_objects_in_scene(commands, registry, frame_idx);

        draw_outlines(commands, registry, frame_idx);

        commands->EndRenderPass();

        // copy_render_targets(commands);
    }

    void DirectLightingPass::create_framebuffer(const glm::uvec2& render_resolution) {
        auto& device = renderer->get_render_backend();

        const auto color_target_create_info = TextureCreateInfo{
            .name = SCENE_COLOR_RENDER_TARGET,
            .usage = TextureUsage::RenderTarget,
            .format = TextureFormat::Rgba16F,
            .width = render_resolution.x,
            .height = render_resolution.y,
            .enable_resource_sharing = true,
        };

        color_target_handle = renderer->create_texture(color_target_create_info);

        const auto object_id_create_info = TextureCreateInfo{
            .name = OBJECT_ID_TARGET,
            .usage = TextureUsage::RenderTarget,
            .format = TextureFormat::R32UInt,
            .width = render_resolution.x,
            .height = render_resolution.y,
            .enable_resource_sharing = true,
        };

        object_id_target_handle = renderer->create_texture(object_id_create_info);

        const auto depth_target_create_info = TextureCreateInfo{
            .name = SCENE_DEPTH_TARGET,
            .usage = TextureUsage::DepthStencil,
            .format = TextureFormat::Depth32,
            .width = render_resolution.x,
            .height = render_resolution.y,
        };

        depth_target_handle = renderer->create_texture(depth_target_create_info);

        const auto& color_target = renderer->get_texture(color_target_handle);
        const auto& object_id_target = renderer->get_texture(object_id_target_handle);
        const auto& depth_target = renderer->get_texture(depth_target_handle);

        color_target_descriptor = device.create_rtv_handle(color_target);
        color_target_access = {.cpuDescriptor = color_target_descriptor.cpu_handle,
                               .BeginningAccess = {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
                                                   .Clear = {.ClearValue = {.Format = DXGI_FORMAT_R32_FLOAT, .Color = {0, 0, 0, 0}}}},
                               .EndingAccess = {.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE}};

        object_id_target_descriptor = device.create_rtv_handle(object_id_target);
        object_id_target_access = {.cpuDescriptor = object_id_target_descriptor.cpu_handle,
                                   .BeginningAccess = {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
                                                       .Clear = {.ClearValue = {.Format = DXGI_FORMAT_R32_UINT, .Color = {0, 0, 0, 0}}}},
                                   .EndingAccess = {.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE}};

        depth_target_descriptor = device.create_dsv_handle(depth_target);
        depth_target_access = {.cpuDescriptor = depth_target_descriptor.cpu_handle,
                               .DepthBeginningAccess = {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
                                                        .Clear = {.ClearValue = {.Format = DXGI_FORMAT_R32_FLOAT,
                                                                                 .DepthStencil = {.Depth = 1.0}}}},
                               .StencilBeginningAccess = {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD},
                               .DepthEndingAccess = {.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE},
                               .StencilEndingAccess = {.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD}};

        render_target_size = render_resolution;

        // auto downsampled_depth_create_info = depth_target_create_info;
        // downsampled_depth_create_info.name = "Depth buffer with mips";
        // downsampled_depth_create_info.usage = ImageUsage::UnorderedAccess;
        //
        // downsampled_depth_target_handle = renderer->create_image(downsampled_depth_create_info);
    }

    TextureHandle DirectLightingPass::get_color_target_handle() const { return color_target_handle; }

    TextureHandle DirectLightingPass::get_object_id_texture() const { return object_id_target_handle; }

    TextureHandle DirectLightingPass::get_depth_target_handle() const { return depth_target_handle; }

    void DirectLightingPass::begin_render_pass(ID3D12GraphicsCommandList4* commands) const {
        const auto color_targets = std::array{color_target_access, object_id_target_access};
        commands->BeginRenderPass(static_cast<UINT>(color_targets.size()),
                                  color_targets.data(),
                                  &depth_target_access,
                                  D3D12_RENDER_PASS_FLAG_NONE);

        D3D12_VIEWPORT viewport{};
        viewport.MinDepth = 0;
        viewport.MaxDepth = 1;
        viewport.Width = static_cast<float>(render_target_size.x);
        viewport.Height = static_cast<float>(render_target_size.y);
        commands->RSSetViewports(1, &viewport);

        D3D12_RECT scissor_rect{};
        scissor_rect.right = static_cast<LONG>(render_target_size.x);
        scissor_rect.bottom = static_cast<LONG>(render_target_size.y);
        commands->RSSetScissorRects(1, &scissor_rect);
    }

    void DirectLightingPass::draw_objects_in_scene(ID3D12GraphicsCommandList4* commands, entt::registry& registry, const Uint32 frame_idx) {
        ZoneScoped;
        PIXScopedEvent(commands, forward_pass_color, "ObjectsPass::draw_objects_in_scene");

        commands->SetPipelineState(standard_pipeline->pso);

        const auto& mesh_storage = renderer->get_static_mesh_store();
        mesh_storage.bind_to_command_list(commands);

        const auto& renderable_view = registry.view<TransformComponent, StandardRenderableComponent>();
        renderable_view.each([&](const auto entity, const TransformComponent& transform, const StandardRenderableComponent& renderable) {
            // TODO: Frustum culling, view distance calculations, etc

            // TODO: Figure out the priority queues to put things in

            // TODO: Record drawcalls into an indirect command buffer rather than recording into the command list

            commands->SetGraphicsRoot32BitConstant(0, static_cast<uint32_t>(entity), RenderBackend::OBJECT_ID_ROOT_CONSTANT_OFFSET);

            commands->SetGraphicsRoot32BitConstant(0, renderable.material.index, RenderBackend::DATA_INDEX_ROOT_CONSTANT_OFFSET);

            const auto model_matrix_index = renderer->add_model_matrix_to_frame(transform.get_model_matrix(registry), frame_idx);
            commands->SetGraphicsRoot32BitConstant(0, model_matrix_index, RenderBackend::MODEL_MATRIX_INDEX_ROOT_CONSTANT_OFFSET);

            commands->DrawIndexedInstanced(renderable.mesh.num_indices, 1, renderable.mesh.first_index, 0, 0);
        });
    }

    void DirectLightingPass::draw_outlines(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) {
        PIXScopedEvent(commands, forward_pass_color, "ObjectsPass::draw_outlines");
        commands->SetPipelineState(outline_pipeline->pso);

        const auto outline_view = registry.view<TransformComponent, StandardRenderableComponent, OutlineRenderComponent>();
        outline_view.each([&](const auto entity,
                              const TransformComponent& transform,
                              const StandardRenderableComponent& renderable,
                              const OutlineRenderComponent& outline) {
            // TODO: Culling and whatnot

            const auto entity_id = static_cast<uint32_t>(entity);
            commands->SetGraphicsRoot32BitConstant(0, entity_id, RenderBackend::OBJECT_ID_ROOT_CONSTANT_OFFSET);

            commands->SetGraphicsRoot32BitConstant(0, outline.material.index, RenderBackend::DATA_INDEX_ROOT_CONSTANT_OFFSET);

            // Intentionally a copy - I want to modify the transform for the outline without modifying the transform for the renderable
            auto outline_transform = transform;

            outline_transform.transform.scale *= outline.outline_scale;

            const auto model_material_index = renderer->add_model_matrix_to_frame(outline_transform.get_model_matrix(registry), frame_idx);
            commands->SetGraphicsRoot32BitConstant(0, model_material_index, RenderBackend::MODEL_MATRIX_INDEX_ROOT_CONSTANT_OFFSET);

            commands->DrawIndexedInstanced(renderable.mesh.num_indices, 1, renderable.mesh.first_index, 0, 0);
        });
    }

    void DirectLightingPass::draw_atmosphere(ID3D12GraphicsCommandList4* commands, entt::registry& registry) const {
        const auto atmosphere_view = registry.view<SkyComponent>();
        if(atmosphere_view.size() > 1) {
            logger->error("May only have one atmospheric sky component in a scene");

        } else {
            PIXScopedEvent(commands, forward_pass_color, "ObjectsPass::draw_atmosphere");

            const auto atmosphere_entity = atmosphere_view.front();
            commands->SetGraphicsRoot32BitConstant(0,
                                                   static_cast<uint32_t>(atmosphere_entity),
                                                   RenderBackend::OBJECT_ID_ROOT_CONSTANT_OFFSET);

            commands->SetPipelineState(atmospheric_sky_pipeline->pso);

            commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            commands->DrawInstanced(3, 1, 0, 0);
        }
    }

    void DirectLightingPass::copy_render_targets(ID3D12GraphicsCommandList4* commands) const {
        const auto& object_id_texture = renderer->get_texture(object_id_target_handle);
        const auto& depth_image = renderer->get_texture(depth_target_handle);
        const auto& downsampled_depth_image = renderer->get_texture(downsampled_depth_target_handle);

        {
            const auto barriers = std::array{
                CD3DX12_RESOURCE_BARRIER::Transition(object_id_texture.resource,
                                                     D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                     D3D12_RESOURCE_STATE_COPY_SOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(depth_image.resource,
                                                     D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                     D3D12_RESOURCE_STATE_COPY_SOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(downsampled_depth_image.resource,
                                                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                     D3D12_RESOURCE_STATE_COPY_DEST),
            };
            commands->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
        }

        const D3D12_TEXTURE_COPY_LOCATION src_copy_location{.pResource = depth_image.resource,
                                                            .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                                                            .SubresourceIndex = 0};
        const D3D12_TEXTURE_COPY_LOCATION dst_copy_location{.pResource = downsampled_depth_image.resource,
                                                            .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                                                            .SubresourceIndex = 0};
        const D3D12_BOX src_box{.left = 0,
                                .top = 0,
                                .front = 0,
                                .right = depth_image.width,
                                .bottom = depth_image.height,
                                .back = depth_image.depth};
        commands->CopyTextureRegion(&dst_copy_location, 0, 0, 0, &src_copy_location, &src_box);

        {
            const auto barriers = std::array{
                CD3DX12_RESOURCE_BARRIER::Transition(object_id_texture.resource,
                                                     D3D12_RESOURCE_STATE_COPY_SOURCE,
                                                     D3D12_RESOURCE_STATE_RENDER_TARGET),
                CD3DX12_RESOURCE_BARRIER::Transition(depth_image.resource,
                                                     D3D12_RESOURCE_STATE_COPY_SOURCE,
                                                     D3D12_RESOURCE_STATE_DEPTH_WRITE),
                CD3DX12_RESOURCE_BARRIER::Transition(downsampled_depth_image.resource,
                                                     D3D12_RESOURCE_STATE_COPY_DEST,
                                                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            };
            commands->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
        }

        renderer->get_spd().generate_mip_chain_for_texture(downsampled_depth_image.resource, commands);
    }
} // namespace sanity::engine::renderer
