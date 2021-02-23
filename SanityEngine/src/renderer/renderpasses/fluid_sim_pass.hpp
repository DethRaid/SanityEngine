#pragma once

#include "renderer/hlsl/fluid_sim.hpp"
#include "renderer/render_pass.hpp"
#include "renderer/rhi/compute_pipeline_state.hpp"
#include "renderer/rhi/per_frame_buffer.hpp"
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

        /**
         * @brief All the fluid volumes we're updating this frame
         */
        Rx::Vector<FluidVolumeHandle> active_fluid_volumes;

        /**
         * @brief Tracks the state of read/write textures for each active fluid volume
         */
        Rx::Vector<GpuFluidVolumeState> fluid_volume_states;

        PerFrameBuffer advection_params_array;
        PerFrameBuffer buoyancy_params_array;
        PerFrameBuffer impulse_params_array;

        ComPtr<ID3D12PipelineState> advection_pipeline;
        ComPtr<ID3D12PipelineState> buoyancy_pipeline;
        ComPtr<ID3D12PipelineState> impulse_pipeline;

        ComPtr<ID3D12CommandSignature> fluid_sim_dispatch_signature;
        Rx::Vector<FluidSimDispatch> fluid_sim_dispatches;
        PerFrameBuffer fluid_sim_dispatch_command_buffers;

        void load_shaders();

        void create_indirect_command_signature(RenderBackend& backend);

        void create_buffers(Uint32 num_gpu_frames);

        void set_buffer_indices(ID3D12GraphicsCommandList* commands, Uint32 frame_idx) const;

        void execute_simulation_step(
            ID3D12GraphicsCommandList* commands,
            BufferHandle data_buffer_handle,
            ID3D12PipelineState* pipeline,
            Rx::Function<void(GpuFluidVolumeState&, Rx::Vector<D3D12_RESOURCE_BARRIER>& barriers)> synchronize_volume);

        void apply_advection(ID3D12GraphicsCommandList* commands);

        void apply_buoyancy(ID3D12GraphicsCommandList* commands);

        void apply_impulse(ID3D12GraphicsCommandList* commands);

        void barrier_and_swap(TextureHandle handles[2], Rx::Vector<D3D12_RESOURCE_BARRIER>& barriers) const;
    };
} // namespace sanity::engine::renderer
