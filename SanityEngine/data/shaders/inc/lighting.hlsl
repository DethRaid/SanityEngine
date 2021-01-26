#pragma once

#include "StandardMaterial.hlsli"
#include "atmospheric_scattering.hlsl"
#include "brdf.hlsl"
#include "noise.hlsli"
#include "shadow.hlsl"
#include "sky.hlsl"
#include "standard_root_signature.hlsl"

#define BYTES_PER_VERTEX 9

#define STANDARD_ROUGHNESS 0.01

#define RAY_START_OFFSET_ALONG_NORMAL 0.01f

#define GI_BOOST PI

uint3 get_indices(uint triangle_index) {
    const uint base_index = (triangle_index * 3);
    const int address = (base_index * 4);
    return indices.Load3(address);
}

StandardVertex get_vertex(int address) {
    StandardVertex v;
    v.position = asfloat(vertices.Load3(address));
    address += (3 * 4);
    v.normal = asfloat(vertices.Load3(address));
    address += (3 * 4);
    v.color = asfloat(vertices.Load4(address));
    address += (4); // Vertex colors are only only one byte per component
    v.texcoord = asfloat(vertices.Load2(address));

    return v;
}

StandardVertex get_vertex_attributes(uint triangle_index, float2 barycentrics) {
    uint3 indices = get_indices(triangle_index);

    StandardVertex v0 = get_vertex((indices[0] * BYTES_PER_VERTEX) * 4);
    StandardVertex v1 = get_vertex((indices[1] * BYTES_PER_VERTEX) * 4);
    StandardVertex v2 = get_vertex((indices[2] * BYTES_PER_VERTEX) * 4);

    StandardVertex v;
    v.position = v0.position + barycentrics.x * (v1.position - v0.position) + barycentrics.y * (v2.position - v0.position);
    v.normal = v0.normal + barycentrics.x * (v1.normal - v0.normal) + barycentrics.y * (v2.normal - v0.normal);
    v.color = v0.color + barycentrics.x * (v1.color - v0.color) + barycentrics.y * (v2.color - v0.color);
    v.texcoord = v0.texcoord + barycentrics.x * (v1.texcoord - v0.texcoord) + barycentrics.y * (v2.texcoord - v0.texcoord);

    return v;
}

struct SanityRayHit {
    float3 position;
    float3 normal;
    float3 brdf_result;
};

float3 direct_analytical_light(const in SurfaceInfo surface, const in Light light, const in float3 view_vector) {
    float3 light_vector;
    if(light.type == LIGHT_TYPE_DIRECTIONAL) {
        light_vector = normalize(light.direction_or_location);

    } else {
        light_vector = normalize(surface.location - light.direction_or_location);
    }

    const float3 outgoing_light = brdf(surface, light_vector, view_vector) * light.color;

    float shadow = 1.0;
    if(length(outgoing_light) > 0) {
        shadow = saturate(raytrace_shadow(light_vector, light.angular_size, surface.location, surface.normal));
    }

    return outgoing_light * shadow;
}

/*!
 * \brief Sample the light that's coming from a given direction to a given point
 *
 * \return A float4 where the rgb are the incoming light and the a is 1 if we hit a surface, 0 is we're sampling the sky
 */
float4 get_incoming_light(in float3 ray_origin,
                          in float3 direction,
                          in Light sun,
                          inout RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_CULL_BACK_FACING_TRIANGLES> query,
                          out StandardVertex vertex,
                          out MaterialData material) {

    RayDesc ray;
    ray.Origin = ray_origin;
    ray.TMin = 0.001;
    ray.Direction = direction;
    ray.TMax = 1000; // TODO: Pass this in with a CB

    // Set up work
    query.TraceRayInline(raytracing_scene, 0, 0xFF, ray);

    // Actually perform the trace
    query.Proceed();

    if(query.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
        float t = query.CommittedRayT();
        if(t <= 0) {
            // Ray is stuck
            return 0;
        }

        uint triangle_index = query.CommittedPrimitiveIndex();
        float2 barycentrics = query.CommittedTriangleBarycentrics();
        vertex = get_vertex_attributes(triangle_index, barycentrics);

        uint material_id = query.CommittedInstanceContributionToHitGroupIndex();
        material = material_buffer[material_id];

        const SurfaceInfo surface = get_surface_info(vertex, material);

        const float3 lit_hit_surface = direct_analytical_light(surface, sun, ray.Direction);

        return float4(lit_hit_surface, 1.0);

    } else {
        // Sample the atmosphere
        const float3 sky_direction = direction * float3(1, -1, -1);
        const float3 sky = get_sky_in_direction(sky_direction);
        return float4(sky, 0);
    }
}

float2 wang_hash(uint2 seed) {
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return float2(seed) / (int((uint(1) << 31)) - 1);
}

float3 raytrace_reflections(const in SurfaceInfo original_surface,
                            const in float3 eye_vector,
                            const in float2 noise_texcoord,
                            const in Light sun,
                            const in Texture2D noise) {
    const uint num_specular_rays = 2;

    const uint num_bounces = 2;

    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_CULL_BACK_FACING_TRIANGLES> query;

    SurfaceInfo surface = original_surface;

    float3 view_vector = eye_vector;

    float3 reflection = 0;

    for(uint ray_idx = 1; ray_idx <= num_specular_rays; ray_idx++) {
        float3 brdf_accumulator = 1;
        float3 light_sample = 0;

        for(uint bounce_idx = 1; bounce_idx <= num_bounces; bounce_idx++) {
            float3 ray_direction = get_random_vector_aligned_to_normal(surface.normal,
                                                                       noise_texcoord,
                                                                       bounce_idx + ray_idx * num_bounces,
                                                                       num_bounces * num_specular_rays);

            ray_direction = normalize(lerp(surface.normal, ray_direction, surface.roughness));

            brdf_accumulator *= brdf(surface, ray_direction, view_vector);

            StandardVertex hit_vertex;
            MaterialData hit_material;

            float4 incoming_light = get_incoming_light(surface.location, ray_direction, sun, query, hit_vertex, hit_material);

            light_sample += brdf_accumulator * incoming_light.rgb;

            if(incoming_light.a > 0.05) {
                // set up next ray
                surface = get_surface_info(hit_vertex, hit_material);

                view_vector = ray_direction;

            } else {
                // Ray escaped into the sky
                break;
            }
        }

        reflection += light_sample / (float) num_bounces;
    }

    return reflection / (float) num_specular_rays;
}

/*!
 * \brief Calculate the raytraced indirect light that hits a surface
 */
float3 raytrace_global_illumination(const in SurfaceInfo original_surface,
                                    const in float3 eye_vector,
                                    const in float2 noise_texcoord,
                                    const in Light sun,
                                    const in Texture2D noise) {
    const uint num_indirect_rays = 19;

    const uint num_bounces = 3;

    // TODO: In theory, we should walk the ray to collect all transparent hits that happen closer than the closest opaque hit, and filter
    // the opaque hit's light through the transparent surfaces. This will be implemented l a t e r when I feel more comfortable with ray
    // shaders

    float3 indirect_light = 0;

    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_CULL_BACK_FACING_TRIANGLES> query;

    SurfaceInfo surface = original_surface;

    float3 view_vector = eye_vector;

    for(uint ray_idx = 1; ray_idx <= num_indirect_rays; ray_idx++) {
        float3 brdf_accumulator = 1;
        float3 light_sample = 0;

    	uint num_light_samples = 1;
        for(uint bounce_idx = 1; bounce_idx <= num_bounces; bounce_idx++) {
            const float3 ray_direction = get_random_vector_aligned_to_normal(surface.normal,
                                                                             noise_texcoord,
                                                                             bounce_idx + ray_idx * num_bounces,
                                                                             num_bounces * num_indirect_rays);

            brdf_accumulator *= brdf(surface, ray_direction, view_vector);

            StandardVertex hit_vertex;
            MaterialData hit_material;

            const float4 incoming_light = get_incoming_light(surface.location, ray_direction, sun, query, hit_vertex, hit_material);

            light_sample += brdf_accumulator * incoming_light.rgb;

            if(incoming_light.a > 0.05) {
                // set up next ray

                surface = get_surface_info(hit_vertex, hit_material);

                view_vector = ray_direction;

            } else {
                // Ray escaped into the sky
                break;
            }

        	num_light_samples++;
        }

        indirect_light += light_sample / (float) num_light_samples;
    }

    const float3 indirect_diffuse = indirect_light / (float) num_indirect_rays;

    return indirect_diffuse * GI_BOOST;
}

float3 get_total_reflected_light(const Camera camera, const SurfaceInfo surface, Texture2D noise) {
    // Transform worldspace position into viewspace position
    const float4 position_viewspace = mul(camera.view, float4(surface.location, 1));

    // Treat viewspace position as the view vector
    const float3 view_vector_viewspace = normalize(position_viewspace.xyz);

    // Transform view vector into worldspace
    float3 view_vector_worldspace = normalize(mul(camera.inverse_view, float4(view_vector_viewspace, 0)).xyz);
    view_vector_worldspace.z *= -1;

    Light sun = lights[SUN_LIGHT_INDEX];

    // Calculate how much of the sun's light gets through the atmosphere
    const float3 direction_to_sun = normalize(sun.direction_or_location * float3(-1, 1, -1));
    const float sun_strength = length(sun.color);
    sun.color = sun_and_atmosphere(direction_to_sun, sun_strength, direction_to_sun);

    const float3 direct_light = direct_analytical_light(surface, sun, view_vector_worldspace);
    // return direct_light;

    const float4 location_ndc = mul(camera.projection, position_viewspace) + 4196.f;

	const PerFrameData frame_data = per_frame_data[0];

    float2 noise_tex_size;
    noise.GetDimensions(noise_tex_size.x, noise_tex_size.y);
    float2 noise_texcoord = location_ndc.xy * frame_data.render_size / noise_tex_size;
    // const float2 offset = noise.Sample(bilinear_sampler, noise_texcoord * per_frame_data[0].time_since_start).rg;
    // noise_texcoord *= offset;

    const float3 indirect_light = raytrace_global_illumination(surface, view_vector_worldspace, noise_texcoord, sun, noise);
     return indirect_light;
    return direct_light + indirect_light;

    const float3 reflection = raytrace_reflections(surface, view_vector_worldspace, noise_texcoord, sun, noise);
    // return reflection;

    return direct_light + indirect_light + reflection;
}
