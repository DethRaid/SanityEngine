#pragma once

#include "renderer/renderpass.hpp"
#include "renderer/hlsl/fluid_sim_data.hpp"
#include "renderer/rhi/compute_pipeline_state.hpp"
#include "renderer/rhi/render_backend.hpp"
#include "rx/core/ptr.h"

using Microsoft::WRL::ComPtr;

namespace sanity::engine::renderer {
    class Renderer;
    /**
     * @brief Executes all fluid simulations, including fire, smoke, and water
     */
    class FluidSimPass : public RenderPass {
    public:
        explicit FluidSimPass(Renderer& renderer_in);

        void render(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) override;

    private:
        Renderer* renderer{nullptr};

    	Rx::Vector<FluidSimData> all_fluid_sim_data;
        Rx::Vector<BufferHandle> fluid_sim_data_buffers;

        ComPtr<ID3D12PipelineState> fluid_sim_pipeline;
        ComPtr<ID3D12CommandSignature> fluid_sim_command_signature;
    };
} // namespace sanity::engine::renderer
