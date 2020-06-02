#pragma once

#include "standard_root_signature.hlsl"

#define NUM_SHADOW_RAYS 1

// from https://gist.github.com/keijiro/ee439d5e7388f3aafc5296005c8c3f33
// Rotation with angle (in radians) and axis
float3x3 AngleAxis3x3(float angle, float3 axis) {
    float c, s;
    sincos(angle, s, c);

    float t = 1 - c;
    float x = axis.x;
    float y = axis.y;
    float z = axis.z;

    return float3x3(t * x * x + c,
                    t * x * y - s * z,
                    t * x * z + s * y,
                    t * x * y + s * z,
                    t * y * y + c,
                    t * y * z - s * x,
                    t * x * z - s * y,
                    t * y * z + s * x,
                    t * z * z + c);
}

float raytrace_shadow(Light light, float3 position_worldspace, float2 noise_texcoord, Texture2D noise) {
    // Shadow ray query
    RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> q;

    float shadow_strength = 1.0;

    for(uint i = 1; i <= NUM_SHADOW_RAYS; i++) {
        // Random cone with the same angular size as the sun
        float3 random_vector = float3(noise.Sample(bilinear_sampler, noise_texcoord * i).rgb * 2.0 - 1.0);

        float3 projected_vector = random_vector - (-light.direction * dot(-light.direction, random_vector));
        float random_angle = random_vector.z * i * light.angular_size;
        float3x3 rotation_matrix = AngleAxis3x3(random_angle, projected_vector);
        float3 ray_direction = mul(rotation_matrix, -light.direction);

        RayDesc ray;
        ray.Origin = position_worldspace;
        ray.TMin = 0.01; // Slight offset so we don't self-intersect. TODO: Make this slope-scaled
        ray.Direction = ray_direction;
        ray.TMax = 1000; // TODO: Pass this in with a CB

        // Set up work
        q.TraceRayInline(raytracing_scene,
                         RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES,
                         0xFF,
                         ray);

        // Actually perform the trace
        q.Proceed();

        if(q.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
            shadow_strength -= 1.0f / NUM_SHADOW_RAYS;
        }
    }

    return shadow_strength;
}