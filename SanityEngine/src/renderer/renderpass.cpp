#include "renderpass.hpp"

#include "renderer/rhi/d3dx12.hpp"
#include "rx/core/log.h"

RX_LOG("RenderPass", logger);

namespace sanity::engine::renderer {
    const Rx::Map<TextureHandle, Rx::Optional<BeginEndState>>& RenderPass::get_texture_states() const { return texture_states; }

    void RenderPass::add_resource_usage(const TextureHandle handle, const D3D12_RESOURCE_STATES states) {
        add_resource_usage(handle, states, states);
    }

    void RenderPass::add_resource_usage(const TextureHandle handle, D3D12_RESOURCE_STATES begin_states, D3D12_RESOURCE_STATES end_states) {
        if(auto* usage_ptr = texture_states.find(handle); usage_ptr != nullptr) {
            *usage_ptr = Rx::Pair{begin_states, end_states};
            return;
        }

        texture_states.insert(handle, Rx::Pair{begin_states, end_states});
    }

    void RenderPass::remove_resource_usage(const TextureHandle handle) {
        if(auto* usage_ptr = texture_states.find(handle); usage_ptr != nullptr) {
            *usage_ptr = Rx::nullopt;
        }
    }
} // namespace sanity::engine::renderer
