#pragma once

#include "interop.hpp"
#include "renderer/rhi/resources.hpp"

#if __cplusplus
#include "renderer/handles.hpp"

namespace sanity::engine::renderer {
#endif

    struct FluidVolume {
    	// R - density
    	// G - temperature
    	// B - reaction
    	// A - phi
        TextureHandle texture_1_handle;

    	// R - velocity
    	// G - pressure
        TextureHandle texture_2_handle;
    	
        uint3 size;
        uint padding;
    };
	
    struct GpuFluidVolume {
        // R - density
        // G - temperature
        // B - reaction
        // A - phi
        uint texture_1_idx;

        // R - velocity
        // G - pressure
        uint texture_2_idx;
    	
        uint3 size;
        uint padding;
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
    using FluidVolumeHandle = GpuResourceHandle<FluidVolume>;
}
#endif
