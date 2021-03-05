// Use ifndef because dxc doesn't support #pragma once https://github.com/microsoft/DirectXShaderCompiler/issues/676
#ifndef SHARED_STRUCTS_HPP
#define SHARED_STRUCTS_HPP

#include "interop.hpp"

#if __cplusplus
namespace sanity::engine::renderer {
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
        float delta_time;
        float time_since_start;
        uint frame_count;

    	float ambient_temperature;

        uint camera_buffer_index;
        uint light_buffer_index;
        uint vertex_data_buffer_index;
        uint index_buffer_index;

        uint noise_texture_idx;
        uint sky_texture_idx;

        uint2 render_size;
    };
	
    struct PostprocessingMaterial {
        uint scene_output_image;
    };
	
    struct StandardPushConstants {
        /*!
         * \brief Index of the per-frame-data buffer
         */
        uint frame_constants_buffer_index;

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

    /**
     * @brief Data for an object's drawcall
     *
     * Contains pointers to the object's data struct and model matrix, along with the object's entity ID
     */
    struct ObjectDrawData {
        uint data_idx;
        uint entity_id;
        uint model_matrix_idx;
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

#endif