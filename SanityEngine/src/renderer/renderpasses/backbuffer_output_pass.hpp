#pragma once

#include "renderer/renderpass.hpp"
#include "rhi/render_pipeline_state.hpp"
#include <rx/core/ptr.h>

namespace renderer {
    class Renderer;
    class DenoiserPass;

    class BackbufferOutputPass final : public RenderPass {
    public:
        explicit BackbufferOutputPass(Renderer& renderer_in, const DenoiserPass& denoiser_pass);

        ~BackbufferOutputPass() override = default;

        void execute(ID3D12GraphicsCommandList4* commands, entt::registry& registry, uint32_t frame_idx) override;

    private:
        Renderer* renderer;

        Rx::Ptr<rhi::RenderPipelineState> backbuffer_output_pipeline;

        Rx::Ptr<rhi::Buffer> backbuffer_output_material_buffer;
    };
} // namespace renderer
