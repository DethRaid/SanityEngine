struct FullscreenVertexOutput {
    float4 position : SV_POSITION;
    float3 position_viewspace : VIEWPOS;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {
    uint texture_idx;
};

#include "inc/standard_root_signature.hlsl"

#define LUMA(color) float3(color.r * 0.2126, color.g * 0.7152, color.b * 0.0722)

float4 main(FullscreenVertexOutput vertex) : SV_TARGET {
    MaterialData material = material_buffer[constants.material_index];
    Texture2D scene_output = textures[material.texture_idx];
    float4 color = scene_output.Sample(bilinear_sampler, vertex.texcoord);

    float luma = LUMA(color);
    float tonemap_factor = luma / (luma + 1.0);

    float3 correct_color = pow(color.rgb, (1.0 / 2.2).xxx);
    return float4(correct_color, color.a);
}
