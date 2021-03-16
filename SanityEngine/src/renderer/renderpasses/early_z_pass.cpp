#include "early_z_pass.hpp"

#include "renderer/renderer.hpp"

namespace sanity::engine::renderer {
    EarlyDepthPass::EarlyDepthPass(Renderer& renderer_in, const glm::uvec2& output_size) : renderer{&renderer_in} {
        depth_buffer = renderer->create_texture(TextureCreateInfo{.name = "Depth Buffer",
                                                                  .usage = TextureUsage::DepthStencil,
                                                                  .format = TextureFormat::Depth32,
                                                                  .width = output_size.x,
                                                                  .height = output_size.y,
                                                                  .depth = 1});

        set_resource_usage(depth_buffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }

    void EarlyDepthPass::record_work(ID3D12GraphicsCommandList4* commands,
                                     entt::registry& registry,
                                     const Uint32 frame_idx,
                                     const float delta_time) {
        // Empty for now, will fill in later
    }

    TextureHandle EarlyDepthPass::get_depth_buffer() const { return depth_buffer; }
} // namespace sanity::engine::renderer
