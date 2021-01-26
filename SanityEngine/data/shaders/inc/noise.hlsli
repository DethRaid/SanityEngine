#pragma once

#include "standard_root_signature.hlsl"

#define PI 3.141592
#define TAU 2 * PI
#define PHI 1.618033988749894

#define GOLDEN_ANGLE TAU / PHI / PHI

#define NUM_TEMPORAL_SAMPLES 13

float2 fermatsSpiralGoldenN(float index, float total) {
    float theta = index * GOLDEN_ANGLE;
    return float2(sin(theta), cos(theta)) * sqrt(index / total);
}

float2 fermatsSpiralGoldenS(float index, float total) {
    float theta = index * GOLDEN_ANGLE;
    return float2(sin(theta), cos(theta)) * pow(index / total, 2);
}

// from http://www.rorydriscoll.com/2009/01/07/better-sampling/
float3 CosineSampleHemisphere(float2 uv) {
    float r = sqrt(uv.x);
    float theta = 2 * PI * uv.y;
    float x = r * cos(theta);
    float z = r * sin(theta);
    return float3(x, sqrt(max(0, 1 - uv.x)), z);
}

// Adapted from https://github.com/NVIDIA/Q2RTX/blob/9d987e755063f76ea86e426043313c2ba564c3b7/src/refresh/vkpt/shader/utils.glsl#L240
float3x3 construct_ONB_frisvad(float3 normal) {
    float3x3 ret;
    ret[1] = normal;
    if(normal.z < -0.999805696f) {
        ret[0] = float3(0.0f, -1.0f, 0.0f);
        ret[2] = float3(-1.0f, 0.0f, 0.0f);
    } else {
        float a = 1.0f / (1.0f + normal.z);
        float b = -normal.x * normal.y * a;
        ret[0] = float3(1.0f - normal.x * normal.x * a, b, -normal.x);
        ret[2] = float3(b, 1.0f - normal.y * normal.y * a, -normal.y);
    }
    return ret;
}

float3 get_random_vector_aligned_to_normal(const in float3 normal,
                                           const in float2 base_noise_texcoord,
                                           const in float index,
                                           const in float total) {
    const PerFrameData frame_data = per_frame_data[0];
    const uint frame_index = frame_data.frame_count % NUM_TEMPORAL_SAMPLES;

	const float offset = total * NUM_TEMPORAL_SAMPLES;

    const float2 noise_sample_location = base_noise_texcoord +
                                         fermatsSpiralGoldenS(index + frame_index * total,
                                                              total * NUM_TEMPORAL_SAMPLES);

    Texture2D noise = textures[0];

    float2 nums;
    nums.x = noise.Sample(bilinear_sampler, noise_sample_location).r;
    nums.y = noise.Sample(bilinear_sampler, noise_sample_location * PI).r;

    float3 random_vector = CosineSampleHemisphere(nums);
    const float3x3 onb = transpose(construct_ONB_frisvad(normal));
    random_vector = normalize(mul(onb, random_vector));

    if(dot(normal, random_vector) < 0) {
        random_vector *= -1;
    }

    return random_vector;
}
