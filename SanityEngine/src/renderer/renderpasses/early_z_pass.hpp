#pragma once

#include "renderer/render_pass.hpp"

namespace sanity::engine::renderer
{
    class EarlyZPass : public RenderPass
    {
    public:
        EarlyZPass(Renderer& renderer_in, const glm::uvec2& output_size);

        void record_work(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) override;

        [[nodiscard]] TextureHandle get_z_buffer() const;

    private:
        Renderer* renderer;

        TextureHandle z_buffer;
    };
}
