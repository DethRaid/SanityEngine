// Use ifndef because dxc doesn't support #pragma once https://github.com/microsoft/DirectXShaderCompiler/issues/676
#ifndef STANDARD_ROOT_SIGNATURE_HPP
#define STANDARD_ROOT_SIGNATURE_HPP 

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

StandardPushConstants constants;

// Registers 0 - 15: raytracing stuff that can't be bindlessly accessed

/*!
 * \brief Acceleration structure for all the objects that we can raytrace against
 */
RaytracingAccelerationStructure raytracing_scene : register(t0);

// Register 16: Resource arrays

// Space 0 - 15: buffers

ByteAddressBuffer srv_buffers[] : register(t16, space0);

RWByteAddressBuffer uav_buffers[] : register(u16, space1);

// Space 16 - 31: Texture2D

Texture2D textures[] : register(t16, space16);

RWTexture2D uav_textures2d_rgba[] : register(t16, space20);

// Space 32 - 47: Texture3D

Texture3D textures3d[] : register(t16, space32);

RWTexture3D<float> uav_textures3d_r[] : register(u16, space36);

RWTexture3D<float2> uav_textures3d_rg[] : register(u16, space37);

RWTexture3D<float4> uav_textures3d_rgba[] : register(u16, space38);

// Helpers

FrameConstants get_frame_constants() {
    ByteAddressBuffer frame_data_buffer = srv_buffers[constants.frame_constants_buffer_index];
    return frame_data_buffer.Load<FrameConstants>(0);
}

Light get_light(const uint index) {
    FrameConstants per_frame_data = get_frame_constants();
    ByteAddressBuffer lights_buffer = srv_buffers[per_frame_data.light_buffer_index];
    return lights_buffer.Load<Light>(sizeof(Light) * index);
}

Camera get_camera(const uint index) {
    FrameConstants per_frame_data = get_frame_constants();
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
    ByteAddressBuffer data_buffer = srv_buffers[constants.data_buffer_index];  \
	const uint read_offset = sizeof(Type) * index;   \
	const Type varname = data_buffer.Load<Type>(read_offset);

#define GET_CURRENT_DATA(Type, varname) GET_DATA(Type, constants.data_index, varname)

#endif
