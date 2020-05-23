struct FullscreenVertexOutput {
    float4 position : SV_POSITION;
    float3 position_clipspace : CLIPPOS;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {
    uint accumulation_texture_idx;
    uint scene_output_texture_idx;
    uint scene_depth_texture_idx;
};

#include "inc/standard_root_signature.hlsl"

const static float ACCUMULATION_POWER = 0.025;

// TODO: Sample from the accumulation buffer from the camera's viewpoint last frame 

float4 main(FullscreenVertexOutput input) : SV_TARGET {
    MaterialData material = material_buffer[constants.material_index];

    Texture2D accumulation_texture = textures[material.accumulation_texture_idx];
    Texture2D scene_output_texture = textures[material.scene_output_texture_idx];

    float3 accumulated_color = accumulation_texture.Sample(point_sampler, input.texcoord).rgb;
    float4 scene_color = scene_output_texture.Sample(point_sampler, input.texcoord);

    float3 final_color = lerp(accumulated_color, scene_color.rgb, ACCUMULATION_POWER);

	return float4(final_color, scene_color.a);
}