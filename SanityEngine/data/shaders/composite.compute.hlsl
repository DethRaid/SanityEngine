#include "compositing.hpp"
#include "inc/standard_root_signature.hlsl"

[numthreads(COMPOSITING_NUM_THREADS, COMPOSITING_NUM_THREADS, 1)]
void main(const uint3 id : SV_DispatchThreadID) {
    GET_CURRENT_DATA(CompositingTextures, texture_indices);

    const Texture2D scene_color_texture = textures[texture_indices.direct_lighting_idx];
    const Texture2D fluid_color_texture = textures[texture_indices.fluid_color_idx];

    RWTexture2D<float4> output_texture = uav_textures2d_rgba[texture_indices.output_idx];
    
    const float4 color_sample = scene_color_texture[id.xy];
    const float4 fluid_color_sample = fluid_color_texture[id.xy];

    const float4 output_color = fluid_color_sample * fluid_color_sample.a + color_sample * (1.f - fluid_color_sample.a);

    output_texture[id.xy] = output_color;
}