#include "compositing_pass.hpp"

#include "loading/shader_loading.hpp"
#include "renderer/renderer.hpp"

namespace sanity::engine::renderer {
    CompositingPass::CompositingPass(Renderer& renderer_in,
                                     const glm::uvec2& output_size,
                                     const RenderpassHandle<DirectLightingPass>& direct_light_pass,
                                     const RenderpassHandle<FluidSimPass>& fluid_sim_pass)
        : renderer{&renderer_in} {
        direct_lighting_texture_handle = direct_light_pass->get_color_target_handle();
        fluid_color_target_handle = fluid_sim_pass->get_color_target_handle();
        set_resource_usage(direct_lighting_texture_handle, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        set_resource_usage(fluid_color_target_handle, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        create_output_texture(output_size);

        create_material_buffer();

        create_pipeline();
    }

    void CompositingPass::record_work(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) {}

    TextureHandle CompositingPass::get_output_handle() const { return output_handle; }

    void CompositingPass::create_output_texture(const glm::uvec2& output_size) {
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

        material_buffer = renderer->create_buffer(BufferCreateInfo{.name = "Compositing material", .usage = BufferUsage::ConstantBuffer, .size = sizeof(CompositingTextures)}, &data);
    }

    void CompositingPass::create_pipeline() {
        const auto& shader = load_shader("composite.compute");
        auto& backend = renderer->get_render_backend();
        composite_pipeline = backend.create_compute_pipeline_state(shader);
    }
} // namespace sanity::engine::renderer
