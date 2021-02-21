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

    struct FrameConstants {
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

    struct StandardPushConstants {
        /*!
         * \brief Index of the per-frame-data buffer
         */
        uint per_frame_data_buffer_index;

        /*!
         * \brief Index of the camera that will render this draw
         */
        uint camera_index;

        /*!
         * \brief Index in the global buffers array of the buffer that holds our data
         */
        uint data_buffer_index;

        /*!
         * \brief Index of the material data for the current draw
         */
        uint data_index;

        /*!
         * \brief Index of the buffer with model matrices for this draw
         */
        uint model_matrix_buffer_index;

        /*!
         * \brief Index of our model matrix within the currently bound buffer
         */
        uint model_matrix_index;

        /*!
         * \brief Identifier for the object currently being rendered. Guaranteed to be unique for each object
         */
        uint object_id;
    };

    struct IndirectDrawCommandWithRootConstant {
        uint constant;
        uint vertex_count;
        uint instance_count;
        uint start_index_location;
        int base_vertex_location;
        uint start_instance_location;
    };
#if __cplusplus
}
#endif
