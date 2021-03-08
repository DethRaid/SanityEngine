#include "compositing_pass.hpp"


#include "TracyD3D12.hpp"
#include "loading/shader_loading.hpp"
#include "renderer/hlsl/compositing.hpp"
#include "renderer/renderer.hpp"

namespace sanity::engine::renderer {
    CompositingPass::CompositingPass(Renderer& renderer_in,
                                     const glm::uvec2& output_size_in,
                                     const DenoiserPass& denoiser_pass,
                                     const FluidSimPass& fluid_sim_pass)
        : renderer{&renderer_in}, output_size{output_size_in} {
        ZoneScoped;
        direct_lighting_texture_handle = denoiser_pass.get_output_texture();
        fluid_color_target_handle = fluid_sim_pass.get_color_target_handle();
        set_resource_usage(direct_lighting_texture_handle, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        set_resource_usage(fluid_color_target_handle, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        create_output_texture();

        create_material_buffer();

        create_pipeline();
    }

    void CompositingPass::record_work(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) {
        ZoneScoped;
        TracyD3D12Zone(RenderBackend::tracy_render_context, commands, "CompositingPass::record_work");
        PIXScopedEvent(commands, PIX_COLOR_DEFAULT, "CompositingPass::record_work");

        commands->SetPipelineState(composite_pipeline);

        commands->SetComputeRoot32BitConstant(RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                              material_buffer.index,
                                              RenderBackend::DATA_BUFFER_INDEX_ROOT_PARAMETER_OFFSET);
        commands->SetComputeRoot32BitConstant(RenderBackend::ROOT_CONSTANTS_ROOT_PARAMETER_INDEX,
                                              0,
                                              RenderBackend::DATA_INDEX_ROOT_CONSTANT_OFFSET);

        commands->Dispatch(output_size.x / COMPOSITING_NUM_THREADS, output_size.y / COMPOSITING_NUM_THREADS, 1);
    }

    TextureHandle CompositingPass::get_output_handle() const { return output_handle; }

    void CompositingPass::create_output_texture() {
        output_handle = renderer->create_texture(TextureCreateInfo{.name = "Composited Render Target",
                                                                   .usage = TextureUsage::RenderTarget,
                                                                   .format = TextureFormat::Rgba16F,
                                                                   .width = output_size.x,
                                                                   .height = output_size.y,
                                                                   .depth = 1});
        set_resource_usage(output_handle, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    void CompositingPass::create_material_buffer() {
        const CompositingTextures data{
            .direct_lighting_idx = direct_lighting_texture_handle.index,
            .fluid_color_idx = fluid_color_target_handle.index,
            .output_idx = output_handle.index,
        };

        material_buffer = renderer->create_buffer(BufferCreateInfo{.name = "Compositing material",
                                                                   .usage = BufferUsage::ConstantBuffer,
                                                                   .size = sizeof(CompositingTextures)},
                                                  &data);
    }

    void CompositingPass::create_pipeline() {
        const auto& shader = load_shader("composite.compute");
        auto& backend = renderer->get_render_backend();
        composite_pipeline = backend.create_compute_pipeline_state(shader);
    }
} // namespace sanity::engine::renderer
