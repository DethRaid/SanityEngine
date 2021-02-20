#pragma once

#if __cplusplus
#include "core/types.hpp"

namespace sanity::engine::renderer {

    // ReSharper disable CppInconsistentNaming
	
    using uint = Uint32;
    using uint2 = glm::uvec2;

	using float3 = glm::vec3;
    using float4x4 = glm::mat4;
	
    // ReSharper restore CppInconsistentNaming

#endif

    struct Light {
        uint type;
        float3 color;
        float3 direction_or_location;
        float angular_size;
    };
	
    struct Camera {
        float4x4 view;
        float4x4 projection;
        float4x4 inverse_view;
        float4x4 inverse_projection;

        float4x4 previous_view;
        float4x4 previous_projection;
        float4x4 previous_inverse_view;
        float4x4 previous_inverse_projection;
    };

    struct PerFrameData {
        float time_since_start;
        uint frame_count;

        uint camera_buffer_index;
        uint light_buffer_index;
        uint vertex_data_buffer_index;
        uint index_buffer_index;

        uint noise_texture_idx;
        uint sky_texture_idx;
    	
        uint2 render_size;
    };

#if __cplusplus
}
#endif
