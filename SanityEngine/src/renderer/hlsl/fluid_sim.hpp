#pragma once

#include "interop.hpp"

#if __cplusplus
namespace sanity::engine::renderer {
#endif

    struct FluidVolume {
        uint density_temperature_reaction_phi_texture_handle;
        uint velocity_pressure_texture_handle;
        uint3 size;
        uint padding;
    };

#if __cplusplus
	using FluidVolumeHandle = GpuResourceHandle<FluidVolume>;
	
}
#endif
