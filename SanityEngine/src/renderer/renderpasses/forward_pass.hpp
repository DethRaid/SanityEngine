#pragma once

#include <memory>

#include <spdlog/logger.h>

#include "../../rhi/framebuffer.hpp"
#include "../../rhi/render_pipeline_state.hpp"
#include "../renderpass.hpp"

namespace renderer {
    class ForwardPass final : public virtual RenderPass {
    public:
        ~ForwardPass() override;

        void execute(ID3D12GraphicsCommandList4* commands, entt::registry& registry) override;

    private:
        static std::shared_ptr<spdlog::logger> logger;

        std::unique_ptr<rhi::Framebuffer> scene_framebuffer;

        std::unique_ptr<rhi::RenderPipelineState> standard_pipeline;
        std::unique_ptr<rhi::RenderPipelineState> atmospheric_sky_pipeline;

        void begin_render_pass(ID3D12GraphicsCommandList4* commands) const;

        void draw_objects_in_scene(ID3D12GraphicsCommandList4* commands,
                                   entt::registry& registry,
                                   const rhi::BindGroup& material_bind_group);

        void draw_sky(ID3D12GraphicsCommandList4* command_list, entt::registry& registry) const;
    };
} // namespace renderer
