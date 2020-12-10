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
    if(base_color.a == 0) {
        // Early-out to avoid the expensive raytrace on completely transparent surfaces
        discard;
    }

    Texture2D normal_texture = textures[material.normal_idx];
    float3 texture_normal = normal_texture.Sample(bilinear_sampler, input.texcoord) * 2.0 - 1.0;

    float3x3 normal_matrix = cotangent_frame(input.normal, input.position_worldspace, input.texcoord);
    float3 surface_normal = mul(normal_matrix, texture_normal);

	Texture2D metallic_roughness_texture = textures[material.metallic_roughness_idx];
    float2 metallic_roughness = metallic_roughness_texture.Sample(bilinear_sampler, input.texcoord).gb;

    base_color.rgb = pow(base_color.rgb, 1.0 / 2.2);

    Camera camera = cameras[constants.camera_index];
    Texture2D noise = textures[0];

    float3 total_reflected_light = get_total_reflected_light(camera, input, base_color.rgb, surface_normal, metallic_roughness.y, noise);

    return float4(total_reflected_light, 1);
}
