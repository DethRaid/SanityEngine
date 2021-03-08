#pragma once

#include "denoiser_pass.hpp"
#include "renderer/render_pass.hpp"
#include "renderer/renderpasses/DirectLightingPass.hpp"
#include "renderer/renderpasses/fluid_sim_pass.hpp"
#include "renderer/renderpasses/renderpass_handle.hpp"

namespace sanity::engine::renderer {
    /**
     * @brief Composites together all the object rendering passes so that postprocessing can operate on all the light that reaches the
     * viewer
     */
    class CompositingPass : public RenderPass {
    public:
        CompositingPass(Renderer& renderer_in,
                        const glm::uvec2& output_size_in,
                        const DenoiserPass& denoiser_pass,
                        const FluidSimPass& fluid_sim_pass);

        void record_work(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) override;

        [[nodiscard]] TextureHandle get_output_handle() const;

    private:
        Renderer* renderer;

        TextureHandle direct_lighting_texture_handle;
        TextureHandle fluid_color_target_handle;

        TextureHandle output_handle;

        BufferHandle material_buffer;

        ComPtr<ID3D12PipelineState> composite_pipeline;
        glm::uvec2 output_size;

        void create_output_texture();

        void create_material_buffer();

        void create_pipeline();
    };
} // namespace sanity::engine::renderer
