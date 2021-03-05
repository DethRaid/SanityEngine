#include "early_z_pass.hpp"

#include "renderer/renderer.hpp"

namespace sanity::engine::renderer {
    EarlyZPass::EarlyZPass(Renderer& renderer_in, const glm::uvec2& output_size) : renderer{&renderer_in} {
        z_buffer = renderer->create_texture(TextureCreateInfo{.name = "Depth Buffer",
                                                              .usage = TextureUsage::DepthStencil,
                                                              .format = TextureFormat::Depth32,
                                                              .width = output_size.x,
                                                              .height = output_size.y,
                                                              .depth = 1});

        set_resource_usage(z_buffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }

    void EarlyZPass::record_work(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) {
        // Empty for now, will fill in later
    }

    TextureHandle EarlyZPass::get_z_buffer() const { return z_buffer; }
} // namespace sanity::engine::renderer
