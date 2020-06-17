#pragma once

#include <glm/fwd.hpp>
#include <rx/core/ptr.h>

#include "renderer/handles.hpp"
#include "renderer/renderpass.hpp"
#include "rhi/framebuffer.hpp"
#include "rhi/render_pipeline_state.hpp"

class World;

namespace renderer {
    struct BindGroup;
    class RenderDevice;
} // namespace rhi

namespace renderer {
    class Renderer;

    class ForwardPass final : public virtual RenderPass {
    public:
        explicit ForwardPass(Renderer& renderer_in, const glm::uvec2& render_resolution, const World& world_in);

        ~ForwardPass() override;

#pragma region RenderPass
        void execute(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) override;
#pragma endregion

        [[nodiscard]] TextureHandle get_color_target_handle() const;

        [[nodiscard]] TextureHandle get_depth_target_handle() const;

    private:
        Renderer* renderer;
        const World* world;

        Rx::Ptr<RenderPipelineState> standard_pipeline;
        Rx::Ptr<RenderPipelineState> opaque_chunk_geometry_pipeline;
        Rx::Ptr<RenderPipelineState> chunk_water_pipeline;
        Rx::Ptr<RenderPipelineState> atmospheric_sky_pipeline;

        TextureHandle color_target_handle;
        TextureHandle depth_target_handle;
        Rx::Ptr<Framebuffer> scene_framebuffer;

        void create_framebuffer(const glm::uvec2& render_resolution);

        void begin_render_pass(ID3D12GraphicsCommandList4* commands) const;

        void draw_objects_in_scene(ID3D12GraphicsCommandList4* commands,
                                   entt::registry& registry,
                                   const BindGroup& material_bind_group,
                                   Uint32 frame_idx);

        void draw_chunks(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx);

        void draw_atmosphere(ID3D12GraphicsCommandList4* commands, entt::registry& registry) const;
    };
} // namespace renderer
