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

struct StandardPushConstants {
    /*!
     * \brief Index of the camera that will render this draw
     */
    uint camera_index;

    /*!
     * \brief Index of the material data for the current draw
     */
    uint material_index;

    /*!
     * \brief Index of our model matrix within the currently bound buffer
     */
    uint model_matrix_index;

	/*!
	 * \brief Identifier for the object currently being rendered. Guaranteed to be unique for each object
	 */
	uint object_id;
} constants;

/*!
 * \brief Array of all the materials
 */
StructuredBuffer<Camera> cameras : register(t0);

/*!
 * \brief Array of all the materials
 */
StructuredBuffer<MaterialData> material_buffer : register(t1);

/*!
 * \brief Array of all the lights in the scene
 *
 * When I get forward+ rendering working, there will be a separate buffer for which lights affect which object
 */
StructuredBuffer<Light> lights : register(t2);

/*!
 * \brief Acceleration structure for all the objects that we can raytrace against
 */
RaytracingAccelerationStructure raytracing_scene : register(t3);

ByteAddressBuffer indices : register(t4);
ByteAddressBuffer vertices : register(t5);

/*!
 * \brief Data which is constant for the entire frame
 */
StructuredBuffer<PerFrameData> per_frame_data : register(t6);

/*!
 * \brief Buffer of all the model matrices that are relevant for the current thing being rendered
 */
StructuredBuffer<float4x4> model_matrices : register(t7);

/*!
 * \brief Array of all the textures that are available for a shader to sample from
 */
Texture2D textures[] : register(t16);
