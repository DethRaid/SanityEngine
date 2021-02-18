#pragma once

#include "renderer/renderpass.hpp"
#include "renderer/rhi/compute_pipeline_state.hpp"
#include "renderer/rhi/render_device.hpp"
#include "rx/core/ptr.h"

using Microsoft::WRL::ComPtr;

namespace sanity::engine::renderer {
    class Renderer;
    /**
     * @brief Executes all fluid simulations, including fire, smoke, and water
     */
    class FluidSimPass : public RenderPass {
    public:
    	static inline const Uint32 ROOT_CONSTANTS_INDEX = 0;
        static inline const Uint32 FLUID_SIM_PARAMS_BUFFER_INDEX = 1;
        static inline const Uint32 TEXTURES_ARRAY_INDEX = 2;

        static inline const Uint32 FLUID_VOLUME_INDEX_ROOT_CONSTANT_OFFSET = 0;

        explicit FluidSimPass(Renderer& renderer_in);

        void render(ID3D12GraphicsCommandList4* commands, entt::registry& registry, Uint32 frame_idx) override;

    private:
        Renderer* renderer{nullptr};

        ComPtr<ID3D12PipelineState> fluid_sim_pipeline;
        ComPtr<ID3D12RootSignature> fluid_sim_root_sig;
        ComPtr<ID3D12CommandSignature> fluid_sim_command_signature;
    };
} // namespace sanity::engine::renderer
