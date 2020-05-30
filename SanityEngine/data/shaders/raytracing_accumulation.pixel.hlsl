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

float LinearizeDepth(float depth) {
    float nearZ = 0.01f;
    float farZ = 1000.0f; // Fake far because infinite projection matrix
    return (2.0 * nearZ) / (farZ + nearZ - depth * (farZ - nearZ));
}

float4 main(FullscreenVertexOutput input) : SV_TARGET {
    Camera camera = cameras[constants.camera_index];
    MaterialData material = material_buffer[constants.material_index];

    Texture2D accumulation_texture = textures[material.accumulation_texture_idx];
    Texture2D scene_output_texture = textures[material.scene_output_texture_idx];
    Texture2D depth_texture = textures[material.scene_depth_texture_idx];
    float cur_depth = depth_texture.Sample(point_sampler, input.texcoord).r;

    float4 position_clipspace = float4(float3(input.texcoord, cur_depth) * 2.0 - 1.0, 1);
    float4 position_viewspace = mul(camera.inverse_projection, position_clipspace);
    float4 position_worldspace = mul(camera.inverse_view, position_viewspace);
    position_worldspace /= position_worldspace.w;

    float4 previous_position_viewspace = mul(camera.previous_view, position_worldspace);
    float4 previous_position_clipspace = mul(camera.previous_projection, previous_position_viewspace);
    previous_position_clipspace /= previous_position_clipspace.w;
    float2 previous_texcoord = previous_position_clipspace.xy * 0.5 + 0.5;

    float4 scene_color = scene_output_texture.Sample(point_sampler, input.texcoord);

    // Only add in the previous frame's data if the pixel was onscreen last frame
    if(previous_texcoord.x <= 1.0 && previous_texcoord.x >= 0.0 && previous_texcoord.y <= 1.0 && previous_texcoord.y >= 0.0) {
        float3 accumulated_color = accumulation_texture.Sample(point_sampler, input.texcoord).rgb;

        float3 final_color = lerp(accumulated_color, scene_color.rgb, ACCUMULATION_POWER);

        return float4(final_color, scene_color.a);

    } else {
        return scene_color;
    }
}