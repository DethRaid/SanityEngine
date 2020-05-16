#define NUM_SHADOW_RAYS 8

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

#include "inc/lighting.hlsl"
#include "inc/standard_root_signature.hlsl"

// from https://gamedev.stackexchange.com/questions/32681/random-number-hlsl
float rand_1_05(in float2 uv) {
    float2 noise = (frac(sin(dot(uv, float2(12.9898, 78.233) * 2.0)) * 43758.5453));
    return abs(noise.x + noise.y) * 0.5;
}

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
    float3 view_vector = normalize(position_viewspace.xyz);

    float3 light_vector_viewspace = normalize(mul(camera.view, float4(-sun.direction, 0)).xyz);
    float3 normal_viewspace = normalize(mul(camera.view, float4(input.normal, 0)).xyz);

    float3 light_from_sun = brdf(albedo.rgb, 0.02, 0.5, normal_viewspace, light_vector_viewspace, view_vector) * sun.color;

    float sun_shadow = 1;

    // Only cast shadow rays if the pixel faces the light source
    if(length(light_from_sun) > 0) {
        // Shadow ray query
        RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;

        Texture2D noise = textures[material.noise_idx];

        uint2 noise_tex_size;
        noise.GetDimensions(noise_tex_size.x, noise_tex_size.y);

        for(uint i = 1; i <= NUM_SHADOW_RAYS; i++) {
            // Random cone with the same angular size as the sun
            float2 noise_texcoord = input.position.xy / float2(noise_tex_size);

            float3 random_vector = float3(noise.Sample(bilinear_sampler, noise_texcoord * i).rgb * 2.0 - 1.0);

            float3 projected_vector = random_vector - (-sun.direction * dot(-sun.direction, random_vector));
            float random_angle = rand_1_05(input.position_worldspace.zy * i) * sun.angular_size;
            float3x3 rotation_matrix = AngleAxis3x3(random_angle, projected_vector);
            float3 ray_direction = mul(rotation_matrix, -sun.direction);

            RayDesc ray;
            ray.Origin = input.position_worldspace;
            ray.TMin = 0.01; // Slight offset so we don't self-intersect. TODO: Make this slope-scaled
            ray.Direction = ray_direction;
            ray.TMax = 1000; // TODO: Pass this in with a CB

            // Set up work
            q.TraceRayInline(raytracing_scene,
                             RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
                             0xFF,
                             ray);

            // Actually perform the trace
            q.Proceed();
            if(q.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
                sun_shadow -= 1.0f / NUM_SHADOW_RAYS;
            }
        }
    }

    float3 final_color = light_from_sun * sun_shadow;

    final_color += albedo.rgb * float3(0.2, 0.2, 0.2);

    return float4(final_color, albedo.a);
}
