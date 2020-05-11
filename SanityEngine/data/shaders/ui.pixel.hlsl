struct VertexOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
};

struct MaterialData {
    uint texture_index;
};

#include "inc/standard_root_signature.hlsl"

float4 main(VertexOutput input) : SV_TARGET
{
    MaterialData material = material_buffer[constants.material_index];
    Texture2D color_texture = textures[material.texture_index];
    float4 color = color_texture.Sample(point_sampler, input.texcoord);
    return input.color; // color * input.color;
}