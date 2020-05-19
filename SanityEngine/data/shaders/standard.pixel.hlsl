struct VertexOutput {
    float4 position : SV_POSITION;
    float3 position_worldspace : WORLDPOS;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {
    uint albedo_idx;
    uint normal_idx;
    uint specular_idx;

    uint noise_idx;
};

#include "inc/atmospheric_scattering.hlsl"
#include "inc/brdf.hlsl"
#include "inc/shadow.hlsl"
#include "inc/standard_root_signature.hlsl"

struct StandardVertex {
    float3 position : Position;
    float3 normal : Normal;
    float4 color : Color;
    float2 texcoord : Texcoord;
};

#define BYTES_PER_VERTEX 9

uint3 get_indices(uint triangle_index) {
    uint base_index = (triangle_index * 3);
    int address = (base_index * 4);
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

    return v0;
}

/*!
 * \brief Calculate the raytraced indirect light that hits a surface
 */
float3 raytraced_indirect_light(
    in float3 position_worldspace, in float3 normal, in float3 albedo, in float2 noise_texcoord, in Light sun, in Texture2D noise) {
    uint num_indirect_rays = 8;

    // TODO: In theory, we should walk the ray to collect all transparent hits that happen closer than the closest opaque hit, and filter
    // the opaque hit's light through the transparent surfaces. This will be implemented l a t e r when I feel more comfortable with ray
    // shaders

    // TODO: Wait for Visual Studio to support Shader Model 6.5 smh
    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_CULL_BACK_FACING_TRIANGLES> query;

    float3 indirect_light = 0;

    for(uint i = 1; i <= num_indirect_rays; i++) {
        // Random hemisphere oriented around the surface's normal
        float3 random_vector = normalize(noise.Sample(bilinear_sampler, noise_texcoord * i).rgb * 2.0 - 1.0);

        float3 projected_vector = random_vector - (normal * dot(normal, random_vector));

        float random_angle = random_vector.z * PI * 2 - PI;
        float3x3 rotation_matrix = AngleAxis3x3(random_angle, projected_vector);
        float3 ray_direction = normalize(mul(rotation_matrix, normal));

        RayDesc ray;
        ray.Origin = position_worldspace;
        ray.TMin = 0.01; // Slight offset so we don't self-intersect. TODO: Make this slope-scaled
        ray.Direction = ray_direction;
        ray.TMax = 1000; // TODO: Pass this in with a CB

        // Set up work
        query.TraceRayInline(raytracing_scene, RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, ray);

        // Actually perform the trace
        query.Proceed();

        if(query.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
            // Sample the BRDF of the hit point
            uint triangle_index = query.CommittedPrimitiveIndex();
            float2 barycentrics = query.CommittedTriangleBarycentrics();
            StandardVertex vertex = get_vertex_attributes(triangle_index, barycentrics);

            // Sample the BRDF at the hit location
            float3 reflected_light = brdf(0.8, 0.02, 0.5, vertex.normal, sun.color, ray.Direction);
            indirect_light += reflected_light;

        } else {
            // Sample the atmosphere

            float3 atmosphere_sample = atmosphere(6471e3,
                                                  ray_direction,
                                                  float3(0, 6371e3, 0),
                                                  -sun.direction,                   // direction of the sun
                                                  22.0f,                            // intensity of the sun
                                                  6371e3,                           // radius of the planet in meters
                                                  6471e3,                           // radius of the atmosphere in meters
                                                  float3(5.5e-6, 13.0e-6, 22.4e-6), // Rayleigh scattering coefficient
                                                  21e-6,                            // Mie scattering coefficient
                                                  8e3,                              // Rayleigh scale height
                                                  1.2e3,                            // Mie scale height
                                                  0.758                             // Mie preferred scattering direction
            );

            if(!any(isnan(atmosphere_sample))) {
                indirect_light += atmosphere_sample;
            }
        }
    }

    return albedo * (indirect_light / num_indirect_rays);
}

float4 main(VertexOutput input) : SV_TARGET {
    MaterialData material = material_buffer[constants.material_index];
    Texture2D texture = textures[material.albedo_idx];
    float4 albedo = texture.Sample(bilinear_sampler, input.texcoord) * input.color;

    if(albedo.a == 0) {
        // Early-out to avoid the expensive raytrace on completely transparent surfaces
        discard;
    }

    Light sun = lights[0]; // The sun is ALWAYS at index 0

    Camera camera = cameras[constants.camera_index];
    float4 position_viewspace = mul(camera.view, float4(input.position_worldspace, 1));
    float3 view_vector_viewspace = normalize(position_viewspace.xyz);
    float3 view_vector_worldspace = mul(camera.inverse_view, float4(view_vector_viewspace, 0)).xyz;

    float3 light_from_sun = brdf(albedo.rgb, 0.02, 0.5, input.normal, -sun.direction, view_vector_worldspace);

    float sun_shadow = 1;

    Texture2D noise = textures[material.noise_idx];
    uint2 noise_tex_size;
    noise.GetDimensions(noise_tex_size.x, noise_tex_size.y);
    float2 noise_texcoord = input.position.xy / float2(noise_tex_size);

    // Only cast shadow rays if the pixel faces the light source
    if(length(light_from_sun) > 0) {
        sun_shadow = raytrace_shadow(sun, input.position_worldspace, noise_texcoord, noise);
    }

    float3 direct_light = light_from_sun * sun_shadow;

    float3 indirect_light = raytraced_indirect_light(input.position_worldspace, input.normal, albedo, noise_texcoord, sun, noise);

    float3 total_reflected_light = indirect_light + direct_light;

    // return float4(indirect_light, 1);
    return float4(total_reflected_light, albedo.a);
}
