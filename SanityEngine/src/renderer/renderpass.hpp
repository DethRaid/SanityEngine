#pragma once

#include <d3d12.h>

#include "core/types.hpp"
#include "entt/entity/fwd.hpp"
#include "rx/core/vector.h"

class World;

namespace renderer {
    struct ResourceUsage {
        ID3D12Resource* resource{nullptr};

        D3D12_RESOURCE_STATES state{D3D12_RESOURCE_STATE_COMMON};
    };

    /*!
     * \brief Simple abstraction for a render pass
     */
    class RenderPass {
    public:
        virtual ~RenderPass() = default;

        virtual void render(ID3D12GraphicsCommandList* commands, entt::registry& registry, Uint32 frame_idx, const World& world) = 0;

    protected:
        void add_resource_usage(const ResourceUsage& usage);

    private:
        Rx::Vector<ResourceUsage> used_resources;

        void issue_pre_pass_barriers(ID3D12GraphicsCommandList* commands);

        void issue_post_pass_barriers(ID3D12GraphicsCommandList* commands);
    };
} // namespace renderer
