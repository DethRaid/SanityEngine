#include "renderpass.hpp"

#include "renderer/rhi/d3dx12.hpp"
#include "rx/core/log.h"

RX_LOG("RenderPass", logger);

namespace renderer {
    const Rx::Map<TextureHandle, Rx::Pair<D3D12_RESOURCE_STATES, D3D12_RESOURCE_STATES>>& RenderPass::get_texture_states() const {
        return texture_states;
    }

    void RenderPass::add_resource_usage(TextureHandle handle, D3D12_RESOURCE_STATES begin_states, D3D12_RESOURCE_STATES end_states) {
        if(end_states = D3D12_RESOURCE_STATE_COMMON) {
            end_states = begin_states;
        }

        if(auto* usage = texture_states.find(handle)) {
            logger->error("Texture with handle %d already has known usages", handle.index);
            return;
        }

        texture_states.insert(handle, {begin_states, end_states});
    }
} // namespace renderer
