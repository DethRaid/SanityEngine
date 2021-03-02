struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
};

struct MaterialData {};

#include "inc/standard_root_signature.hlsl"

float4 main(VertexShaderOutput input) : SV_TARGET {
    Texture2D color_texture = textures[constants.data_index];
    const float4 color = color_texture.Sample(point_sampler, input.texcoord);
    return color * input.color;
}
