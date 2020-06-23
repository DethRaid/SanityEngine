#pragma once

#include <rx/core/ptr.h>

#include "renderer/renderpass.hpp"
#include "rhi/render_pipeline_state.hpp"

namespace renderer {
    class Renderer;
    class DenoiserPass;

    class BackbufferOutputPass final : public virtual RenderPass {
    public:
        explicit BackbufferOutputPass(Renderer& renderer_in, const DenoiserPass& denoiser_pass);

        ~BackbufferOutputPass() override = default;

        void render(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx, const World& world) override;

    private:
        Renderer* renderer;

        Uint64 denoiser_pass_color{PIX_COLOR(91, 133, 170)};

        Rx::Ptr<RenderPipelineState> backbuffer_output_pipeline;

        Rx::Ptr<Buffer> backbuffer_output_material_buffer;
    };
} // namespace renderer
