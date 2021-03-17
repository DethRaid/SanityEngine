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

    // TODO: Separate structs for the per-dispatch state and the per-volume parameters
    struct GpuFluidVolumeState {
        // Index 0 is the read texture, index 1 is the write texture

        uint density_textures[2];
        uint temperature_textures[2];
        uint reaction_textures[2];
        uint velocity_textures[2];
        uint pressure_textures[2];
        uint temp_data_buffer;

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
#if __cplusplus
}
#endif

#endif
