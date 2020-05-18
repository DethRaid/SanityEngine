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
    RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_CULL_BACK_FACING_TRIANGLES> query;

    float3 indirect_light = 0;

    for(uint i = 1; i <= num_indirect_rays; i++) {
        // Random hemisphere oriented around the surface's normal
        float3 random_vector = normalize(noise.Sample(bilinear_sampler, noise_texcoord * i).rgb * 2.0 - 1.0);

        float3 projected_vector = random_vector - (normal * dot(normal, random_vector));
        float random_angle = rand_1_05(random_vector.zy * i) * PI;
        float3x3 rotation_matrix = AngleAxis3x3(random_angle, projected_vector);
        float3 ray_direction = normalize(mul(rotation_matrix, normal));

        RayDesc ray;
        ray.Origin = position_worldspace;
        ray.TMin = 0.01; // Slight offset so we don't self-intersect. TODO: Make this slope-scaled
        ray.Direction = ray_direction;
        ray.TMax = 1000; // TODO: Pass this in with a CB

        // Set up work
        query.TraceRayInline(raytracing_scene,
                             RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
                             0xFF,
                             ray);

        // Actually perform the trace
        query.Proceed();

        if(query.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
            // Sample the BRDF of the hit point

        } else {
            // Sample the atmosphere

            indirect_light += atmosphere(6471e3,
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

    return float4(total_reflected_light, albedo.a);
}
