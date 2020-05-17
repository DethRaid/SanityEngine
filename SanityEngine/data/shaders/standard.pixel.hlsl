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
#include "inc/shadow.hlsl"

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

    float3 light_from_sun = analytical_direct_light(sun, view_vector_worldspace, albedo.rgb, input.normal, 0.02, 0.5);

    float sun_shadow = 1;

    // Only cast shadow rays if the pixel faces the light source
    if(length(light_from_sun) > 0) {
        Texture2D noise = textures[material.noise_idx];
        sun_shadow = raytrace_shadow(sun, input.position_worldspace, input.position.xy, noise);
    }

    float3 direct_light = light_from_sun * sun_shadow;

    float3 indirect_light = float3(0.2, 0.2, 0.2) * albedo.rgb; // raytraced_indirect_light();

    float3 total_reflected_light = indirect_light + direct_light;

    return float4(total_reflected_light, albedo.a);
}
