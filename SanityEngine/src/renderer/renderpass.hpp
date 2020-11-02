#pragma once

#include <d3d12.h>

#include "core/types.hpp"
#include "entt/entity/fwd.hpp"
#include "rx/core/vector.h"

class World;

namespace renderer {
    struct TextureUsage {
        TextureHandle texture{0};

        D3D12_RESOURCE_STATES states{D3D12_RESOURCE_STATE_COMMON};
    };

    /*!
     * \brief Simple abstraction for a render pass
     */
    class RenderPass {
    public:
        virtual ~RenderPass() = default;

        virtual void render(ID3D12GraphicsCommandList* commands, entt::registry& registry, Uint32 frame_idx, const World& world) = 0;

        const Rx::Vector<TextureUsage>& get_used_resources() const;

    protected:
        void add_resource_usage(const TextureUsage& usage);

    private:
        Rx::Vector<TextureUsage> used_textures;
    };
} // namespace renderer
