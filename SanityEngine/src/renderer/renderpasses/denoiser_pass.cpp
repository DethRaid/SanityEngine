#include "denoiser_pass.hpp"

#include "Tracy.hpp"
#include "TracyD3D12.hpp"
#include "loading/shader_loading.hpp"
#include "renderer/renderer.hpp"
#include "renderer/rhi/d3dx12.hpp"
#include "renderer/rhi/render_device.hpp"
#include "rx/core/log.h"

namespace renderer {
    constexpr const char* ACCUMULATION_RENDER_TARGET = "Accumulation target";
    constexpr const char* DENOISED_SCENE_RENDER_TARGET = "Denoised scene color target";

    struct AccumulationMaterial {
        TextureHandle accumulation_texture;
        TextureHandle scene_output_texture;
        TextureHandle scene_depth_texture;
    };

    RX_LOG("DenoiserPass", logger);

    DenoiserPass::DenoiserPass(Renderer& renderer_in, const glm::uvec2& render_resolution, const RaytracedLightingPass& forward_pass)
        : renderer{&renderer_in} {
        ZoneScoped;

        auto& device = renderer->get_render_backend();

        denoising_pipeline = device.create_render_pipeline_state(
            {.name = "Denoising Pipeline",
             .vertex_shader = load_shader("fullscreen.vertex"),
             .pixel_shader = load_shader("raytracing_accumulation.pixel"),
             .depth_stencil_state = {.enable_depth_test = false, .enable_depth_write = false},
             .render_target_formats = Rx::Array{ImageFormat::Rgba32F}});

        create_images_and_framebuffer(render_resolution);

        create_material(forward_pass);

        add_resource_usage(accumulation_target_handle, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
        add_resource_usage(denoised_color_target_handle, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    }

    void DenoiserPass::render(ID3D12GraphicsCommandList4* commands,
                              entt::registry& /* registry */,
                              Uint32 /* frame_idx */,
                              const World& /* world */) {
        ZoneScoped;

        TracyD3D12Zone(RenderBackend::tracy_context, commands, "DenoiserPass::render");
        PIXScopedEvent(commands, PIX_COLOR_DEFAULT, "DenoiserPass::render");

        const auto& accumulation_image = renderer->get_image(accumulation_target_handle);

        {
            const auto
                render_target_access = D3D12_RENDER_PASS_RENDER_TARGET_DESC{.cpuDescriptor = denoised_rtv_handle,
                                                                            .BeginningAccess =
                                                                                {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD},
                                                                            .EndingAccess = {
                                                                                .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE}};

            commands->BeginRenderPass(1, &render_target_access, nullptr, D3D12_RENDER_PASS_FLAG_NONE);
        }

        commands->SetPipelineState(denoising_pipeline->pso.Get());

        commands->SetGraphicsRoot32BitConstant(0, 0, RenderBackend::MATERIAL_INDEX_ROOT_CONSTANT_OFFSET);
        commands->SetGraphicsRootShaderResourceView(RenderBackend::MATERIAL_BUFFER_ROOT_PARAMETER_INDEX,
                                                    denoiser_material_buffer->resource->GetGPUVirtualAddress());

        commands->DrawInstanced(3, 1, 0, 0);

        commands->EndRenderPass();

        const auto& denoised_image = renderer->get_image(denoised_color_target_handle);

        {
            const auto barriers = Rx::Array{CD3DX12_RESOURCE_BARRIER::Transition(accumulation_image.resource.Get(),
                                                                                 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                                                                 D3D12_RESOURCE_STATE_COPY_DEST),
                                            CD3DX12_RESOURCE_BARRIER::Transition(denoised_image.resource.Get(),
                                                                                 D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                                 D3D12_RESOURCE_STATE_COPY_SOURCE)};
            commands->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
        }

        {
            const auto src_copy_location = D3D12_TEXTURE_COPY_LOCATION{.pResource = denoised_image.resource.Get(),
                                                                       .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                                                                       .SubresourceIndex = 0};

            const auto dst_copy_location = D3D12_TEXTURE_COPY_LOCATION{.pResource = accumulation_image.resource.Get(),
                                                                       .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                                                                       .SubresourceIndex = 0};

            const auto copy_box = D3D12_BOX{.right = denoised_image.width, .bottom = denoised_image.height, .back = 1};

            commands->CopyTextureRegion(&dst_copy_location, 0, 0, 0, &src_copy_location, &copy_box);
        }
    }

    TextureHandle DenoiserPass::get_output_image() const { return denoised_color_target_handle; }

    void DenoiserPass::create_images_and_framebuffer(const glm::uvec2& render_resolution) {
        auto& device = renderer->get_render_backend();

        {
            const auto color_target_create_info = ImageCreateInfo{
                .name = DENOISED_SCENE_RENDER_TARGET,
                .usage = ImageUsage::RenderTarget,
                .format = ImageFormat::Rgba32F,
                .width = render_resolution.x,
                .height = render_resolution.y,
                .enable_resource_sharing = true,
            };
            denoised_color_target_handle = renderer->create_image(color_target_create_info);

            const auto& denoised_color_target = renderer->get_image(denoised_color_target_handle);
            denoised_rtv_handle = device.create_rtv_handle(denoised_color_target);
        }

        {
            const auto accumulation_target_create_info = ImageCreateInfo{
                .name = ACCUMULATION_RENDER_TARGET,
                .usage = ImageUsage::SampledImage,
                .format = ImageFormat::Rgba32F,
                .width = render_resolution.x,
                .height = render_resolution.y,
                .enable_resource_sharing = true,
            };
            accumulation_target_handle = renderer->create_image(accumulation_target_create_info);
        }
    }

    void DenoiserPass::create_material(const RaytracedLightingPass& forward_pass) {
        auto& device = renderer->get_render_backend();

        const auto scene_color_target_handle = forward_pass.get_color_target_handle();
        const auto scene_depth_target_handle = forward_pass.get_depth_target_handle();

        const auto accumulation_material = AccumulationMaterial{
            .accumulation_texture = accumulation_target_handle,
            .scene_output_texture = scene_color_target_handle,
            .scene_depth_texture = scene_depth_target_handle,
        };

        logger->verbose("Scene output texture idx: %u, Scene depth texture: %u",
                        scene_color_target_handle.index,
                        scene_depth_target_handle.index);

        denoiser_material_buffer = device.create_buffer({.name = "Denoiser material buffer",
                                                         .usage = BufferUsage::ConstantBuffer,
                                                         .size = static_cast<Uint32>(sizeof(AccumulationMaterial))});

        memcpy(denoiser_material_buffer->mapped_ptr, &accumulation_material, sizeof(AccumulationMaterial));
    }
} // namespace renderer
