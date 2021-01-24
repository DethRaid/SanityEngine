struct VertexOutput {
    float4 position : SV_POSITION;
    float3 position_worldspace : WORLDPOS;
    float3 normal_worldspace : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {
    uint albedo_idx;
    uint normal_idx;
    uint specular_idx;

    uint noise_idx;
};

#include "inc/standard_root_signature.hlsl"

float4 main(VertexOutput input) : SV_TARGET {
    MaterialData material = material_buffer[constants.material_index];
    Texture2D albedo_tex = textures[material.albedo_idx];
    float4 albedo = albedo_tex.Sample(bilinear_sampler, input.texcoord);

    // TODO: Figure out a good cutoff value
    if(albedo.a < 1) {
        // Transparent objects don't contribute to the shadowmap
        discard;
    }

	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
