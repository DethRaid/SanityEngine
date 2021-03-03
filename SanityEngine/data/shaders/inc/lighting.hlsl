#pragma once

#include "load_standard_material.hlsl"
#include "atmospheric_scattering.hlsl"
#include "brdf.hlsl"
#include "noise.hlsli"
#include "shadow.hlsl"
#include "sky.hlsl"
#include "standard_root_signature.hlsl"

#define BYTES_PER_VERTEX 9

#define STANDARD_ROUGHNESS 0.01

#define RAY_START_OFFSET_ALONG_NORMAL 0.01f

#define GI_BOOST 1

#define LIGHTING_RAY_FLAGS RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES

uint3 get_indices(uint triangle_index) {
    const uint base_index = (triangle_index * 3);
    const int address = (base_index * 4);
    
	FrameConstants data = get_frame_constants();
	ByteAddressBuffer indices = srv_buffers[data.index_buffer_index];
    return indices.Load3(address);
}

StandardVertex get_vertex(int address) {    
	FrameConstants data = get_frame_constants();
	ByteAddressBuffer vertices = srv_buffers[data.vertex_data_buffer_index];
	
    StandardVertex v;
    v.location = asfloat(vertices.Load3(address));
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
    v.location = v0.location + barycentrics.x * (v1.location - v0.location) + barycentrics.y * (v2.location - v0.location);
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

float3 direct_analytical_light(const in SurfaceInfo surface,
                               const in Light light,
                               const in float3 view_vector,
                               const in float2 noise_texcoord) {
    float3 light_vector;
    float3 light_color;
    if(light.type == LIGHT_TYPE_DIRECTIONAL) {
        light_vector = normalize(light.direction_or_location);
        light_color = light.color;

    } else {
        const float3 vector_from_light = surface.location - light.direction_or_location;
        light_vector = normalize(vector_from_light);

        const float distance_to_light = length(vector_from_light);
        light_color = light.color / (distance_to_light * distance_to_light);
    }

    const float ndotl = dot(surface.normal, light_vector);
    const float3 outgoing_light = ndotl * brdf(surface, light_vector, view_vector) * light_color;

    float shadow = 1.0;
    if(length(outgoing_light) > 0) {
        shadow = saturate(raytrace_shadow(light_vector, light.angular_size, surface.location, surface.normal, noise_texcoord));
    }

    return outgoing_light * shadow;
}

/*!
 * \brief Sample the light that's coming from a given direction to a given point
 *
 * \return A float4 where the rgb are the incoming light and the a is 1 if we hit a surface, 0 is we're sampling the sky
 */
float4 get_incoming_light(const in float3 ray_origin,
                          const in float3 direction,
                          const in float3 surface_normal,
                          const in Light sun,
                          const in float2 noise_texcoord,
                          inout RayQuery<LIGHTING_RAY_FLAGS> query,
                          out StandardVertex vertex,
                          out StandardMaterial material) {

    const float cos_theta = dot(direction, surface_normal);

    RayDesc ray;
    ray.Origin = ray_origin;
    ray.TMin = 0.01f * (1.0 - cos_theta); // Slight offset so we don't self-intersect
    ray.Direction = direction * float3(1.f, 1.f, -1.f);
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

        const uint triangle_index = query.CommittedPrimitiveIndex();
        const float2 barycentrics = query.CommittedTriangleBarycentrics();
        vertex = get_vertex_attributes(triangle_index, barycentrics);

        uint material_id = query.CommittedInstanceContributionToHitGroupIndex();
        GET_DATA(StandardMaterial, material_id, hit_material)
    	material = hit_material;

        const SurfaceInfo surface = get_surface_info(vertex, material);

        const float3 lit_hit_surface = direct_analytical_light(surface, sun, ray.Direction, noise_texcoord);

        return float4(lit_hit_surface, 1.0);

    } else {
        // Sample the atmosphere
        const float3 sky_direction = direction * float3(1, -1, -1);
        const float3 sky = get_sky_in_direction(sky_direction);
        return float4(sky, 0);
    }
}

/*!
 * \brief Calculate the raytraced indirect light that hits a surface
 */
float3 raytrace_global_illumination(const in SurfaceInfo original_surface,
                                    const in float3 eye_vector,
                                    const in float2 noise_texcoord,
                                    const in Light sun) {
    const uint num_indirect_rays = 5;

    const uint num_bounces = 3;

    // TODO: In theory, we should walk the ray to collect all transparent hits that happen closer than the closest opaque hit, and filter
    // the opaque hit's light through the transparent surfaces. This will be implemented l a t e r when I feel more comfortable with ray
    // shaders

    float3 indirect_light = 0;

    RayQuery<LIGHTING_RAY_FLAGS> query;

    SurfaceInfo surface = original_surface;

    float3 view_vector = eye_vector;

    for(uint ray_idx = 1; ray_idx <= num_indirect_rays; ray_idx++) {
        float3 brdf_accumulator = 1;
        float3 light_sample = 0;

        uint num_light_samples = 1;
        for(uint bounce_idx = 0; bounce_idx < num_bounces; bounce_idx++) {
            const float3 ray_direction = get_random_vector_aligned_to_normal(surface.normal,
                                                                             noise_texcoord,
                                                                             bounce_idx + ray_idx * num_bounces,
                                                                             num_bounces * num_indirect_rays);

            const float pdf = saturate(dot(ray_direction, surface.normal));
            brdf_accumulator *= brdf_single_ray(surface, ray_direction, -view_vector) / pdf;

            StandardVertex hit_vertex;
            StandardMaterial hit_material;

            const float4 incoming_light = get_incoming_light(surface.location,
                                                             ray_direction,
                                                             surface.normal,
                                                             sun,
                                                             noise_texcoord,
                                                             query,
                                                             hit_vertex,
                                                             hit_material);
            const float3 reflected_light = brdf_accumulator * incoming_light.rgb;
            if(any(isnan(reflected_light))) {
                // Something went wrong. Abort this ray and try the next one
                break;
            }

            light_sample += reflected_light;

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

float3 raytrace_reflections(const in SurfaceInfo original_surface,
                            const in float3 eye_vector,
                            const in float2 noise_texcoord,
                            const in Light sun) {
    const uint num_specular_rays = 2;

    const uint num_bounces = 3;

    RayQuery<LIGHTING_RAY_FLAGS> query;

    SurfaceInfo surface = original_surface;

    float3 view_vector = eye_vector;

    float3 reflection = 0;

    for(uint ray_idx = 1; ray_idx <= num_specular_rays; ray_idx++) {
        float3 brdf_accumulator = 1;
        float3 light_sample = 0;

        uint bounce_idx = 0;
        for(bounce_idx = 1; bounce_idx <= num_bounces; bounce_idx++) {

            float3 normal_of_reflection = get_random_vector_aligned_to_normal(surface.normal,
                                                                              noise_texcoord,
                                                                              bounce_idx + ray_idx * num_bounces,
                                                                              num_bounces * num_specular_rays);

            normal_of_reflection = normalize(lerp(surface.normal, normal_of_reflection, surface.roughness));

            const float3 ray_direction = normalize(reflect(view_vector, normal_of_reflection));

            const float pdf = PDF_GGX(sqrt(surface.roughness), surface.normal, ray_direction, -view_vector);
            brdf_accumulator *= brdf_single_ray(surface, ray_direction, -view_vector) / pdf;

            StandardVertex hit_vertex;
            StandardMaterial hit_material;

            float4 incoming_light = get_incoming_light(surface.location,
                                                       ray_direction,
                                                       surface.normal,
                                                       sun,
                                                       noise_texcoord,
                                                       query,
                                                       hit_vertex,
                                                       hit_material);
            const float3 reflected_light = brdf_accumulator * incoming_light.rgb;
            if(any(isnan(reflected_light))) {
                // Something went wrong. Abort this ray and try the next one
                break;
            }

            light_sample += reflected_light;

            if(incoming_light.a > 0.05) {
                // set up next ray
                surface = get_surface_info(hit_vertex, hit_material);

                view_vector = ray_direction;

            } else {
                bounce_idx++;

                // Ray escaped into the sky
                break;
            }
        }

        bounce_idx++;

        reflection += light_sample / (float) bounce_idx;
    }

    return reflection / (float) num_specular_rays;
}

float3 get_total_reflected_light(const Camera camera, const SurfaceInfo surface) {
    // Transform worldspace position into viewspace position
    const float4 position_viewspace = mul(camera.view, float4(surface.location, 1));

    // Treat viewspace position as the view vector
    const float3 view_vector_viewspace = normalize(position_viewspace.xyz);

    // Transform view vector into worldspace
    float3 view_vector_worldspace = normalize(mul(camera.inverse_view, float4(view_vector_viewspace, 0)).xyz);
    view_vector_worldspace.z *= -1;

    Light sun = get_light(SUN_LIGHT_INDEX);

    // Calculate how much of the sun's light gets through the atmosphere
    const float3 direction_to_sun = normalize(sun.direction_or_location * float3(1, -1, 1));
    const float sun_strength = length(sun.color);
    sun.color = sun_and_atmosphere(direction_to_sun, sun_strength, direction_to_sun);

    float4 location_ndc = mul(camera.projection, position_viewspace);
    location_ndc /= location_ndc.w;

    const FrameConstants frame_data = get_frame_constants();

    Texture2D noise = textures[frame_data.noise_texture_idx];

    float2 noise_tex_size;
    noise.GetDimensions(noise_tex_size.x, noise_tex_size.y);
    const float2 noise_texcoord = location_ndc.xy * 4.f * frame_data.render_size / noise_tex_size;

    const float3 direct_light = direct_analytical_light(surface, sun, -view_vector_worldspace, noise_texcoord);
    const float3 indirect_light = raytrace_global_illumination(surface, view_vector_worldspace, noise_texcoord, sun);
    const float3 reflection = raytrace_reflections(surface, view_vector_worldspace, noise_texcoord, sun);
    
    return direct_light + indirect_light + reflection;
}
