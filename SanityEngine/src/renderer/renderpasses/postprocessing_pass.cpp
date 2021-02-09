#include "postprocessing_pass.hpp"

#include "Tracy.hpp"
#include "TracyD3D12.hpp"
#include "loading/shader_loading.hpp"
#include "renderer/renderer.hpp"
#include "renderer/renderpasses/denoiser_pass.hpp"
#include "renderer/rhi/render_device.hpp"
#include "rx/core/log.h"

namespace sanity::engine::renderer {
    struct BackbufferOutputMaterial {
        TextureHandle scene_output_image;
    };

    RX_LOG("PostprocessingPass", logger);

    PostprocessingPass::PostprocessingPass(Renderer& renderer_in, const DenoiserPass& denoiser_pass) : renderer{&renderer_in} {
        auto& device = renderer->get_render_backend();

        const auto create_info = RenderPipelineStateCreateInfo{
            .name = "Backbuffer output",
            .vertex_shader = load_shader("fullscreen.vertex"),
            .pixel_shader = load_shader("postprocessing.pixel"),
            .render_target_formats = Rx::Array{TextureFormat::Rgba8},
        };

        postprocessing_pipeline = device.create_render_pipeline_state(create_info);

        postprocessing_materials_buffer = device.create_buffer(BufferCreateInfo{.name = "Postprocessing materials buffer",
                                                                                .usage = BufferUsage::StagingBuffer,
                                                                                .size = sizeof(BackbufferOutputMaterial)});

        const auto material = BackbufferOutputMaterial{.scene_output_image = denoiser_pass.get_output_texture()};

        memcpy(postprocessing_materials_buffer->mapped_ptr, &material, sizeof(BackbufferOutputMaterial));

        add_resource_usage(material.scene_output_image, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        logger->verbose("Initialized backbuffer output pass");
    }

    void PostprocessingPass::render(ID3D12GraphicsCommandList4* commands, entt::registry& /* registry */, Uint32 /* frame_idx */) {
        if(!output_texture_handle.is_valid()) {
            return;
        }

        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_context, commands, "PostprocessingPass::render");
        PIXScopedEvent(commands, postprocessing_pass_color, "PostprocessingPass::render");

        auto& device = renderer->get_render_backend();

        const auto
            render_target_access = D3D12_RENDER_PASS_RENDER_TARGET_DESC{.cpuDescriptor = output_rtv_handle.cpu_handle,
                                                                        .BeginningAccess =
                                                                            {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
                                                                             .Clear = {.ClearValue = {.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                                                                                      .Color = {0, 0, 0, 0}}}},
                                                                        .EndingAccess = {
                                                                            .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE}};

        commands->BeginRenderPass(1, &render_target_access, nullptr, D3D12_RENDER_PASS_FLAG_NONE);

        const auto& output_texture = renderer->get_texture(output_texture_handle);

        D3D12_VIEWPORT viewport{};
        viewport.MinDepth = 0;
        viewport.MaxDepth = 1;
        viewport.Width = static_cast<float>(output_texture.width);
        viewport.Height = static_cast<float>(output_texture.height);
        commands->RSSetViewports(1, &viewport);

        D3D12_RECT scissor_rect{};
        scissor_rect.right = static_cast<LONG>(output_texture.width);
        scissor_rect.bottom = static_cast<LONG>(output_texture.height);
        commands->RSSetScissorRects(1, &scissor_rect);

        commands->SetGraphicsRootShaderResourceView(RenderBackend::MATERIAL_BUFFER_ROOT_PARAMETER_INDEX,
                                                    postprocessing_materials_buffer->resource->GetGPUVirtualAddress());
        commands->SetGraphicsRoot32BitConstant(0, 0, RenderBackend::MATERIAL_INDEX_ROOT_CONSTANT_OFFSET);
        commands->SetPipelineState(postprocessing_pipeline->pso.Get());
        commands->DrawInstanced(3, 1, 0, 0);

        commands->EndRenderPass();
    }

    void PostprocessingPass::set_output_texture(const TextureHandle new_output_texture_handle) {
        if(output_texture_handle.is_valid()) {
            remove_resource_usage(output_texture_handle);
        }

        add_resource_usage(new_output_texture_handle, D3D12_RESOURCE_STATE_RENDER_TARGET);

        output_texture_handle = new_output_texture_handle;
        const auto& output_texture = renderer->get_texture(output_texture_handle);

        output_rtv_handle = renderer->get_render_backend().create_rtv_handle(output_texture);
    }

    TextureHandle PostprocessingPass::get_output_texture() const { return output_texture_handle; }
} // namespace sanity::engine::renderer
