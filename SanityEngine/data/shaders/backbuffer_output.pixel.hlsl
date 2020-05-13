struct FullscreenVertexOutput {
    float4 position : SV_POSITION;
    float3 position_viewspace : VIEWPOS;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {
    uint texture_idx;
};

#include "inc/standard_root_signature.hlsl"

float4 main(FullscreenVertexOutput vertex) : SV_TARGET {
    MaterialData material = material_buffer[constants.material_index];
    Texture2D scene_output = textures[material.texture_idx];
    float4 color = scene_output.Sample(bilinear_sampler, vertex.texcoord);

    float3 correct_color = pow(color.rgb, (1.0 / 2.2).xxx);
    return float4(correct_color, color.a);
}
