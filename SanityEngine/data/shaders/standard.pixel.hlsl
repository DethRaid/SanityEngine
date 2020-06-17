struct VertexOutput {
    float4 position : SV_POSITION;
    float3 position_worldspace : WORLDPOS;
    float3 normal : NORMAL;
    float4 color : COLOR;
    uint material_index : MATERIALINDEX;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {
    uint albedo_idx;
    uint normal_idx;
    uint specular_idx;

    uint noise_idx;
};

#include "inc/lighting.hlsl"

float4 main(VertexOutput input) : SV_TARGET {
    input.normal = normalize(input.normal);

    MaterialData material = material_buffer[constants.material_index];
    Texture2D albedo_texture = textures[material.albedo_idx];
    Texture2D normal_roughness_texture = textures[material.normal_idx];
    float4 albedo = albedo_texture.Sample(bilinear_sampler, input.texcoord) * input.color;

    if(albedo.a == 0) {
        // Early-out to avoid the expensive raytrace on completely transparent surfaces
        discard;
    }

    albedo.rgb = pow(albedo.rgb, 1.0 / 2.2);

    Camera camera = cameras[constants.camera_index];
    Texture2D noise = textures[material.noise_idx];

    float3 total_reflected_light = get_total_reflected_light(camera, input, albedo.rgb, STANDARD_ROUGHNESS, noise);

    return float4(total_reflected_light, 1);
}
