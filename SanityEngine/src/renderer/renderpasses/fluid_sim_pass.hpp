#pragma once

#include "renderer/hlsl/fluid_sim.hpp"
#include "renderer/render_pass.hpp"
#include "renderer/rhi/compute_pipeline_state.hpp"
#include "renderer/rhi/per_frame_buffer.hpp"
#include "renderer/rhi/render_backend.hpp"

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
        void finalize_resources(ID3D12GraphicsCommandList* commands);

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

        BufferRing advection_params_array;
        BufferRing buoyancy_params_array;
        BufferRing emitters_params_array;
        BufferRing extinguishment_params_array;
        BufferRing vorticity_confinement_params_array;
        BufferRing divergence_params_array;
        Rx::Vector<BufferRing> pressure_param_arrays;
        BufferRing projection_param_arrays;

        ComPtr<ID3D12PipelineState> advection_pipeline;
        ComPtr<ID3D12PipelineState> buoyancy_pipeline;
        ComPtr<ID3D12PipelineState> emitters_pipeline;
        ComPtr<ID3D12PipelineState> extinguishment_pipeline;
        ComPtr<ID3D12PipelineState> vorticity_pipeline;
        ComPtr<ID3D12PipelineState> confinement_pipeline;
        ComPtr<ID3D12PipelineState> divergence_pipeline;
        ComPtr<ID3D12PipelineState> jacobi_pressure_solver_pipeline;
        ComPtr<ID3D12PipelineState> projection_pipeline;

        ComPtr<ID3D12CommandSignature> fluid_sim_dispatch_signature;
        Rx::Vector<FluidSimDispatch> fluid_sim_dispatches;
        BufferRing fluid_sim_dispatch_command_buffers;

        void record_fire_simulation_updates(ID3D12GraphicsCommandList* commands, Uint32 frame_idx);

        void advance_fire_sim_params_arrays();

        void load_shaders();

        void create_indirect_command_signature();

        void set_buffer_indices(ID3D12GraphicsCommandList* commands, Uint32 frame_idx) const;

        void execute_simulation_step(
            ID3D12GraphicsCommandList* commands,
            const BufferRing& data_buffer,
            const ComPtr<ID3D12PipelineState>& pipeline,
            Rx::Function<void(GpuFluidVolumeState&, Rx::Vector<D3D12_RESOURCE_BARRIER>& barriers)> synchronize_volume);

        void apply_advection(ID3D12GraphicsCommandList* commands);

        void apply_buoyancy(ID3D12GraphicsCommandList* commands);

        void apply_emitters(ID3D12GraphicsCommandList* commands);

        void apply_extinguishment(ID3D12GraphicsCommandList* commands);
    	
        void compute_vorticity_confinement(ID3D12GraphicsCommandList* commands);

        void compute_divergence(ID3D12GraphicsCommandList* commands);

        void compute_pressure(ID3D12GraphicsCommandList* commands);

        void compute_projection(ID3D12GraphicsCommandList* commands);

        void barrier_and_swap(TextureHandle handles[2], Rx::Vector<D3D12_RESOURCE_BARRIER>& barriers) const;

        struct TextureCopyParams {
            D3D12_TEXTURE_COPY_LOCATION source;
            D3D12_TEXTURE_COPY_LOCATION dest;
        };

        void copy_read_texture_to_write_texture(TextureHandle read,
                                            TextureHandle write,
                                            Rx::Vector<D3D12_RESOURCE_BARRIER>& pre_copy_barriers,
                                            Rx::Vector<TextureCopyParams>& copies,
                                            Rx::Vector<D3D12_RESOURCE_BARRIER>& post_copy_barriers) const;
    };
} // namespace sanity::engine::renderer
