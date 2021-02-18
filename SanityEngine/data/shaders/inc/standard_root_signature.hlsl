#pragma once

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

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_SPHERE 1
#define LIGHT_TYPE_RECTANGLE 2
#define LIGHT_TYPE_CYLINDER 3

#define SUN_LIGHT_INDEX 0

struct Light {
    uint type;
    float3 color;
    float3 direction_or_location;
    float angular_size;
};

struct PerFrameData {
    uint2 render_size;
	
    float time_since_start;
    uint frame_count;

    uint camera_buffer_index;
    uint light_buffer_index;
    uint vertex_data_buffer_index;
    uint index_buffer_index;
	
	uint noise_texture_idx;
    uint sky_texture_idx;
};

/*!
 * \brief Point sampler you can use to sample any texture
 */
SamplerState point_sampler : register(s0);

/*!
 * \brief Bilinear sampler you can use to sample any texture
 */
SamplerState bilinear_sampler : register(s1);

/*!
 * \brief Trilinear sampler you can use to sample any texture
 */
SamplerState trilinear_sampler : register(s2);

// Root constants

struct StandardPushConstants {
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
    uint material_index;
    
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
} constants;

// Registers 0 - 15: raytracing stuff that can't be bindlessly accessed

/*!
 * \brief Acceleration structure for all the objects that we can raytrace against
 */
RaytracingAccelerationStructure raytracing_scene : register(t0);

// Register 16: Resource arrays

ByteAddressBuffer srv_buffers[] : register(t16, space0);

RWByteAddressBuffer uav_buffers[] : register(u16, space1);


Texture2D textures[] : register(t16, space2);

Texture3D textures3d[] : register(t16, space3);

RWTexture3D<float4> uav_textures3d_rgba[] : register(u16, space4);

RWTexture3D<float2> uav_textures3d_rg[] : register(u16, space5);



