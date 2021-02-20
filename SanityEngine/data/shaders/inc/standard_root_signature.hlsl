#pragma once

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_SPHERE 1
#define LIGHT_TYPE_RECTANGLE 2
#define LIGHT_TYPE_CYLINDER 3

#define SUN_LIGHT_INDEX 0

#include "shared_structs.hpp"

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

// Helpers

PerFrameData get_per_frame_data() {
    ByteAddressBuffer frame_data_buffer = srv_buffers[0];
    return frame_data_buffer.Load<PerFrameData>(0);
}

Light get_light(const uint index) {
    PerFrameData per_frame_data = get_per_frame_data();
    ByteAddressBuffer lights_buffer = srv_buffers[per_frame_data.light_buffer_index];
    return lights_buffer.Load<Light>(sizeof(Light) * index);
}

Camera get_camera(const uint index) {
    PerFrameData per_frame_data = get_per_frame_data();
    ByteAddressBuffer cameras_buffer = srv_buffers[per_frame_data.camera_buffer_index];
    return cameras_buffer.Load<Camera>(sizeof(Camera) * index);
}

Camera get_current_camera() { return get_camera(constants.camera_index); }

struct NonCrashingMatrix {
	float4x4 m;
};

float4x4 get_model_matrix(const uint index) {
    ByteAddressBuffer model_matrix_buffer = srv_buffers[constants.model_matrix_buffer_index];
    return model_matrix_buffer.Load<NonCrashingMatrix>(sizeof(float4x4) * index).m;
}

float4x4 get_current_model_matrix() { return get_model_matrix(constants.model_matrix_index); }

#define GET_DATA(Type, index, varname) \
    ByteAddressBuffer my_material_buffer = srv_buffers[0];  \
	const uint read_offset = sizeof(Type) * index;   \
	const Type varname = my_material_buffer.Load<Type>(read_offset);

#define GET_CURRENT_DATA(Type, varname) GET_DATA(Type, constants.material_index, varname)
