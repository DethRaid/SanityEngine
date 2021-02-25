#pragma once

#include "interop.hpp"

#if __cplusplus
#include "renderer/rhi/resources.hpp"
#include "renderer/handles.hpp"

namespace sanity::engine::renderer {
#else
#define TextureHandle uint
#endif

#define FLUID_SIM_NUM_THREADS 8

    // TODO: Separate structs for the per-dispatch state and the per-simulation parameters
    struct GpuFluidVolumeState {
        // Index 0 is the read texture, index 1 is the write texture

        TextureHandle density_textures[2];
        TextureHandle temperature_textures[2];
        TextureHandle reaction_textures[2];
        TextureHandle velocity_textures[2];
        TextureHandle pressure_textures[2];
        TextureHandle temp_data_buffer;

        uint3 size;
        uint padding;
        
        float4 dissipation;
        
        float4 decay;

        float buoyancy;

        float weight;
        
        float3 emitter_location;
        
        float emitter_radius;

    	float emitter_strength;
    };

    struct FluidSimDispatch {
        uint fluid_volume_idx;
        uint entity_id;
        uint model_matrix_idx;

        uint thread_group_count_x;
        uint thread_group_count_y;
        uint thread_group_count_z;
    };
#if __cplusplus
}
#endif
