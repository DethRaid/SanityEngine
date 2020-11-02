#include "renderpass.hpp"

#include "renderer/rhi/d3dx12.hpp"

namespace renderer {
    const Rx::Vector<TextureUsage>& RenderPass::get_used_resources() const { return used_textures; }

    void RenderPass::add_resource_usage(const TextureUsage& usage) { used_textures.push_back(usage); }
} // namespace renderer
