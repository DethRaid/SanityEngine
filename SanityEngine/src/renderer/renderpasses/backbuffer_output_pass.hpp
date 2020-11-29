#pragma once

#include "renderer/debugging/pix.hpp"
#include "renderer/renderpass.hpp"
#include "renderer/rhi/render_pipeline_state.hpp"
#include "rx/core/ptr.h"

namespace sanity::engine::renderer {
    class Renderer;
    class DenoiserPass;

    class CopySceneOutputToTexturePass final : public RenderPass {
    public:
        explicit CopySceneOutputToTexturePass(Renderer& renderer_in, const DenoiserPass& denoiser_pass);

        ~CopySceneOutputToTexturePass() override = default;

        void render(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx, const World& world) override;

    private:
        Renderer* renderer;

        Uint64 denoiser_pass_color{PIX_COLOR(91, 133, 170)};

        Rx::Ptr<RenderPipelineState> backbuffer_output_pipeline;

        Rx::Ptr<Buffer> backbuffer_output_material_buffer;
    };
} // namespace renderer
