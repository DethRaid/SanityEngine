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
    uint metallic_roughness_idx;

    uint emission_idx;
};

#include "inc/lighting.hlsl"
#include "inc/normalmapping.hlsl"

float4 main(VertexOutput input) : SV_TARGET {
    input.normal = normalize(input.normal);

    MaterialData material = material_buffer[constants.material_index];

    Texture2D albedo_texture = textures[material.albedo_idx];
    float4 base_color = albedo_texture.Sample(bilinear_sampler, input.texcoord) * input.color;

    // TODO: Separate shader (or a shader variant) for alpha-tested objects
    // if(base_color.a == 0) {
    //     // Early-out to avoid the expensive raytrace on completely transparent surfaces
    //     discard;
    // }

    Texture2D normal_texture = textures[material.normal_idx];
    const float3 texture_normal = normal_texture.Sample(bilinear_sampler, input.texcoord).xyz * 2.0 - 1.0;

    const float3x3 normal_matrix = cotangent_frame(input.normal, input.position_worldspace, input.texcoord);
    const float3 surface_normal = mul(normal_matrix, texture_normal);

    Texture2D metallic_roughness_texture = textures[material.metallic_roughness_idx];
    const float4 metallic_roughness = metallic_roughness_texture.Sample(bilinear_sampler, input.texcoord);

    base_color.rgb = pow(base_color.rgb, 1.0 / 2.2);

    const Camera camera = cameras[constants.camera_index];
    const Texture2D noise = textures[0];

    float3 total_reflected_light = get_total_reflected_light(camera,
                                                             input,
                                                             base_color.rgb,
                                                             surface_normal,
                                                             metallic_roughness.g,
                                                             pow(metallic_roughness.b, 0.5),
                                                             noise);

    return float4(total_reflected_light, 1);
}
