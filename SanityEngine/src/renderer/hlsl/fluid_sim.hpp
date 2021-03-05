// Use ifndef because dxc doesn't support #pragma once https://github.com/microsoft/DirectXShaderCompiler/issues/676
#ifndef FLUID_SIM_HPP
#define FLUID_SIM_HPP

#include "interop.hpp"
#include "shared_structs.hpp"

#if __cplusplus
#include "renderer/handles.hpp"
#include "renderer/rhi/resources.hpp"

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

        uint4 size;

        uint4 voxel_size;

        float4 dissipation;

        float4 decay;

        float buoyancy;

        float weight;

        float4 emitter_location;

        float emitter_radius;

        float emitter_strength;

        float reaction_extinguishment;

        float density_extinguishment_amount;

        float vorticity_strength;
    };

    /**
     * @brief Indirect dispatch command for executing a single fluid sim dispatch
     *
     * All the different steps of the fluid simulation use the same parameters, so using the same struct for them isn't a problem
     */
    struct FluidSimDispatch {
        ObjectDrawData instance_data;

        uint thread_group_count_x;
        uint thread_group_count_y;
        uint thread_group_count_z;
    };

    struct FluidSimDraw {
        ObjectDrawData instance_data;

        uint index_count;
        uint instance_count;
        uint first_index;
        uint first_vertex;
        uint first_instance;
    };
#if __cplusplus
}
#endif

#endif
