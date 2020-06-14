#pragma once

#include <rx/core/ptr.h>

#include "renderer/renderpass.hpp"
#include "rhi/render_pipeline_state.hpp"

namespace renderer {
    class Renderer;
    class DenoiserPass;

    class BackbufferOutputPass final : public RenderPass {
    public:
        explicit BackbufferOutputPass(Renderer& renderer_in, const DenoiserPass& denoiser_pass);

        ~BackbufferOutputPass() override = default;

        void execute(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) override;

    private:
        Renderer* renderer;

        Rx::Ptr<rhi::RenderPipelineState> backbuffer_output_pipeline;

        Rx::Ptr<rhi::Buffer> backbuffer_output_material_buffer;
    };
} // namespace renderer
