#pragma once

#include "interop.hpp"
#include "renderer/rhi/resources.hpp"

#if __cplusplus
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

        uint3 size;
        uint padding;

        /**
         * @brief Dissipation values for density, temperature, reaction, and velocity
         */
        float4 dissipation;

        /**
         * @brief Decay amount for density, temperature, reaction, and velocity
         */
        float4 decay;

        float buoyancy;

        float weight;

        /**
         * @brief Location of a reaction emitter, relative to the fluid volume, expressed in NDC
         */
        float3 emitter_location;

        /**
         * @brief Radius of the emitter, again expressed relative to the fluid volume
         *
         * Radius of 1 means the emitter touches the sides of the volume
         *
         * Eventually we'll have support for arbitrarily shaped emitters, and multiple emitters, and really cool things that will make
         * everyone jealous
         */
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
