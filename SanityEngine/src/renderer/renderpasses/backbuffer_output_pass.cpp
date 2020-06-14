#include "backbuffer_output_pass.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>

#include "loading/shader_loading.hpp"
#include "renderer/renderer.hpp"
#include "renderer/renderpasses/denoiser_pass.hpp"
#include "rhi/render_device.hpp"

namespace renderer {
    struct BackbufferOutputMaterial {
        TextureHandle scene_output_image;
    };

    std::shared_ptr<spdlog::logger> BackbufferOutputPass::logger{spdlog::stdout_color_st("BackbufferOutputPass")};

    BackbufferOutputPass::BackbufferOutputPass(Renderer& renderer_in, const DenoiserPass& denoiser_pass) : renderer{&renderer_in} {
        auto& device = renderer->get_render_device();

        const auto create_info = rhi::RenderPipelineStateCreateInfo{
            .name = "Backbuffer output",
            .vertex_shader = load_shader("fullscreen.vertex"),
            .pixel_shader = load_shader("backbuffer_output.pixel"),
            .render_target_formats = Rx::Array{rhi::ImageFormat::Rgba8},
        };

        backbuffer_output_pipeline = device.create_render_pipeline_state(create_info);

        backbuffer_output_material_buffer = device.create_buffer(rhi::BufferCreateInfo{.name = "Backbuffer output material buffer",
                                                                                       .usage = rhi::BufferUsage::StagingBuffer,
                                                                                       .size = sizeof(BackbufferOutputMaterial)});

        const auto material = BackbufferOutputMaterial{.scene_output_image = denoiser_pass.get_output_image()};

        memcpy(backbuffer_output_material_buffer->mapped_ptr, &material, sizeof(BackbufferOutputMaterial));

        logger->debug("Initialized backbuffer output pass");
    }

    void BackbufferOutputPass::execute(ID3D12GraphicsCommandList4* commands, entt::registry& /* registry */, uint32_t /* frame_idx */) {
        auto& device = renderer->get_render_device();
        const auto* framebuffer = device.get_backbuffer_framebuffer();

        const auto
            render_target_access = D3D12_RENDER_PASS_RENDER_TARGET_DESC{.cpuDescriptor = framebuffer->rtv_handles[0],
                                                                        .BeginningAccess =
                                                                            {.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
                                                                             .Clear = {.ClearValue = {.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                                                                                      .Color = {0, 0, 0, 0}}}},
                                                                        .EndingAccess = {
                                                                            .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE}};

        commands->BeginRenderPass(1, &render_target_access, nullptr, D3D12_RENDER_PASS_FLAG_NONE);

        D3D12_VIEWPORT viewport{};
        viewport.MinDepth = 0;
        viewport.MaxDepth = 1;
        viewport.Width = framebuffer->width;
        viewport.Height = framebuffer->height;
        commands->RSSetViewports(1, &viewport);

        D3D12_RECT scissor_rect{};
        scissor_rect.right = static_cast<LONG>(framebuffer->width);
        scissor_rect.bottom = static_cast<LONG>(framebuffer->height);
        commands->RSSetScissorRects(1, &scissor_rect);

        commands->SetGraphicsRootShaderResourceView(rhi::RenderDevice::material_buffer_root_parameter_index,
                                                    backbuffer_output_material_buffer->resource->GetGPUVirtualAddress());
        commands->SetGraphicsRoot32BitConstant(0, 0, 1);
        commands->SetPipelineState(backbuffer_output_pipeline->pso.Get());
        commands->DrawInstanced(3, 1, 0, 0);

        commands->EndRenderPass();
    }
} // namespace renderer
