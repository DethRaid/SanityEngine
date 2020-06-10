#pragma once

#include <memory>

#include <spdlog/logger.h>

#include "../../rhi/render_pipeline_state.hpp"
#include "../renderpass.hpp"

namespace renderer {
    class Renderer;
    class DenoiserPass;

    class BackbufferOutputPass final : public RenderPass {
    public:
        explicit BackbufferOutputPass(Renderer& renderer_in, const DenoiserPass& denoiser_pass);

        ~BackbufferOutputPass() override = default;

        void execute(ID3D12GraphicsCommandList4* commands, entt::registry& registry, uint32_t frame_idx) override;

    private:
        static std::shared_ptr<spdlog::logger> logger;

        Renderer* renderer;

        std::unique_ptr<rhi::RenderPipelineState> backbuffer_output_pipeline;

        std::unique_ptr<rhi::Buffer> backbuffer_output_material_buffer;
    };
} // namespace renderer
