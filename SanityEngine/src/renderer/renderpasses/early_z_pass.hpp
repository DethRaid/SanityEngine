#pragma once

#include "renderer/render_pass.hpp"

namespace sanity::engine::renderer
{
    class EarlyDepthPass : public RenderPass
    {
    public:
        EarlyDepthPass(Renderer& renderer_in, const glm::uvec2& output_size);

        void record_work(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) override;

        [[nodiscard]] TextureHandle get_depth_buffer() const;

    private:
        Renderer* renderer;

        TextureHandle depth_buffer;
    };
}
