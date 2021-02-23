#pragma once

#include "renderer/hlsl/fluid_sim.hpp"
#include "renderer/render_pass.hpp"
#include "renderer/rhi/compute_pipeline_state.hpp"
#include "renderer/rhi/render_backend.hpp"

using Microsoft::WRL::ComPtr;

namespace sanity::engine::renderer {
    class Renderer;
    /**
     * @brief Executes all fluid simulations, including fire, smoke, and water
     */
    class FluidSimPass : public RenderPass {
    public:
        explicit FluidSimPass(Renderer& renderer_in);

    	void collect_work(entt::registry& registry, Uint32 frame_idx) override;

        void record_work(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) override;

    private:
        Renderer* renderer{nullptr};

        ComPtr<ID3D12PipelineState> fluid_sim_pipeline;
        ComPtr<ID3D12CommandSignature> fluid_sim_command_signature;

    	Rx::Vector<FluidSimDispatch> fluid_sim_dispatches;

    	Rx::Vector<BufferHandle> fluid_sim_dispatch_command_buffers;
    };
} // namespace sanity::engine::renderer
