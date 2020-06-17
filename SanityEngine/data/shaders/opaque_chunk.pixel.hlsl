struct VertexOutput {
    float4 position : SV_POSITION;
    float3 position_worldspace : WORLDPOS;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {
    uint albedo_idx;
};

#include "inc/lighting.hlsl"

float4 main(VertexOutput input) : SV_TARGET {
    input.normal = normalize(input.normal);

    // TODO: A biome-aware procedural texturing system
    MaterialData material = material_buffer[0];
    Texture2D albedo_texture = textures[material.albedo_idx];
    float4 albedo = albedo_texture.Sample(bilinear_sampler, input.texcoord) * input.color;

    if(albedo.a == 0) {
        // Early-out to avoid the expensive raytrace on completely transparent surfaces
        discard;
    }

    albedo.rgb = pow(albedo.rgb, 1.0 / 2.2);

    Camera camera = cameras[constants.camera_index];
    Texture2D noise = textures[0];

    float3 total_reflected_light = get_total_reflected_light(camera, input, albedo.rgb, STANDARD_ROUGHNESS, noise);

    return float4(1, 0, 1, 1);//total_reflected_light, 1);
}
