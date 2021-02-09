struct FullscreenVertexOutput {
    float4 position : SV_POSITION;
    float3 position_clipspace : CLIPPOS;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {
    uint texture_idx;
};

#include "inc/standard_root_signature.hlsl"

#define LUMA(color) (color.r * 0.2126 + color.g * 0.7152 + color.b * 0.0722)

// Rienhard tonemapper from
// https://github.com/Crisspl/IrrlichtBAW/blob/6bc539e05edd5eac36bb2e0e3b7300dba0f39136/ext/ToneMapper/CGLSLToneMappingBuiltinIncludeLoader.h
struct IrrGlslExtToneMapperReinhardParamsT {
    float key_and_manual_linear_exposure;
    float rcp_white2;
};

float3 reinhard_tonemap(IrrGlslExtToneMapperReinhardParamsT params, float3 autoexposed_xyz_color, float extra_neg_ev) {
    float3 tonemapped = autoexposed_xyz_color;
    tonemapped.y *= params.key_and_manual_linear_exposure * exp2(-extra_neg_ev);
    tonemapped.y *= (1.0 + tonemapped.y * params.rcp_white2) / (1.0 + tonemapped.y);
    return tonemapped;
}

// From https://www.shadertoy.com/view/4dBcD1
float3 jodie_robo2_electric_boogaloo(const float3 color) {
    float luma = dot(color, float3(.2126, .7152, .0722));

    // tonemap curve goes on this line
    // (I used robo here)
    float4 rgbl = float4(color, luma) * rsqrt(luma * luma + 1.);

    float3 mappedColor = rgbl.rgb;
    float mappedLuma = rgbl.a;

    float channelMax = max(max(max(mappedColor.r, mappedColor.g), mappedColor.b), 1.);

    // this is just the simplified/optimised math
    // of the more human readable version below
    return ((mappedLuma * mappedColor - mappedColor) - (channelMax * mappedLuma - mappedLuma)) / (mappedLuma - channelMax);

    /*
    const float3 white = float3(1);

    // prevent clipping
    float3 clampedColor = mappedColor / channelMax;

    // x is how much white needs to be mixed with
    // clampedColor so that its luma equals the
    // mapped luma
    //
    // mix(mappedLuma/channelMax,1.,x) = mappedLuma;
    //
    // mix is defined as
    // x*(1-a)+y*a
    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/mix.xhtml
    //
    // (mappedLuma/channelMax)*(1.-x)+1.*x = mappedLuma

    float x = (mappedLuma - mappedLuma * channelMax) / (mappedLuma - channelMax);
    return lerp(clampedColor, white, x);
    */
}

float4 main(FullscreenVertexOutput vertex) : SV_TARGET {
    const MaterialData material = material_buffer[constants.material_index];
    Texture2D scene_output = textures[material.texture_idx];
    const float4 color = scene_output.Sample(bilinear_sampler, vertex.texcoord);

    const float luma = LUMA(color.rgb);
    const float tonemap_factor = 1.0 / (1.0 + luma);
    const float3 correct_color = color.rgb * tonemap_factor;

    // IrrGlslExtToneMapperReinhardParamsT params;
    // params.key_and_manual_linear_exposure = 1.5;
    // params.rcp_white2 = 0.9;
    //
    // float3 correct_color = reinhard_tonemap(params, color, 16.0);
    // float3 correct_color = jodie_robo2_electric_boogaloo(color.rgb);
    return float4(correct_color, color.a);
}
