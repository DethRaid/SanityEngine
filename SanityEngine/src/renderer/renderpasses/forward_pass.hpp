#pragma once

#include <memory>

#include <glm/fwd.hpp>

#include "renderer/handles.hpp"
#include "renderer/renderpass.hpp"
#include "rhi/framebuffer.hpp"
#include "rhi/render_pipeline_state.hpp"

namespace rhi {
    struct BindGroup;
    class RenderDevice;
} // namespace rhi

namespace renderer {
    class Renderer;

    class ForwardPass final : public virtual RenderPass {
    public:
        explicit ForwardPass(Renderer& renderer_in, const glm::uvec2& render_resolution);

        ~ForwardPass() override;

#pragma region RenderPass
        void execute(ID3D12GraphicsCommandList4* commands, entt::registry& registry, uint32_t frame_idx) override;
#pragma endregion

        [[nodiscard]] TextureHandle get_color_target_handle() const;

        [[nodiscard]] TextureHandle get_depth_target_handle() const;

    private:
        Renderer* renderer;

        std::unique_ptr<rhi::RenderPipelineState> standard_pipeline;
        std::unique_ptr<rhi::RenderPipelineState> atmospheric_sky_pipeline;

        TextureHandle color_target_handle;
        TextureHandle depth_target_handle;
        std::unique_ptr<rhi::Framebuffer> scene_framebuffer;

        void create_framebuffer(const glm::uvec2& render_resolution);

        void begin_render_pass(ID3D12GraphicsCommandList4* commands) const;

        void draw_objects_in_scene(ID3D12GraphicsCommandList4* commands,
                                   entt::registry& registry,
                                   const rhi::BindGroup& material_bind_group,
                                   uint32_t frame_idx);

        void draw_atmosphere(ID3D12GraphicsCommandList4* command_list, entt::registry& registry) const;
    };
} // namespace renderer
