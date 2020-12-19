#pragma once

// from https://www.shadertoy.com/view/XtKfRV

float4 cubic(float x) {
    float x2 = x * x;
    float x3 = x2 * x;
    float4 w;
    w.x = -x3 + 3.0 * x2 - 3.0 * x + 1.0;
    w.y = 3.0 * x3 - 6.0 * x2 + 4.0;
    w.z = -3.0 * x3 + 3.0 * x2 + 3.0 * x + 1.0;
    w.w = x3;
    return w / 6.0;
}

/*!
 * \brief Four-sample bicubic filter
 */
float4 bicubic_texture(in Texture2D<float4> tex, in SamplerState samp, in float2 coord) {
    uint width, height, num_levels;
    tex.GetDimensions(0, width, height, num_levels);

	float2 resolution = float2(width, height);

    coord *= resolution;

    float fx = frac(coord.x);
    float fy = frac(coord.y);
    coord.x -= fx;
    coord.y -= fy;

    fx -= 0.5;
    fy -= 0.5;

    float4 xcubic = cubic(fx);
    float4 ycubic = cubic(fy);

    float4 c = float4(coord.x - 0.5, coord.x + 1.5, coord.y - 0.5, coord.y + 1.5);
    float4 s = float4(xcubic.x + xcubic.y, xcubic.z + xcubic.w, ycubic.x + ycubic.y, ycubic.z + ycubic.w);
    float4 offset = c + float4(xcubic.y, xcubic.w, ycubic.y, ycubic.w) / s;

    float4 sample0 = tex.Sample(samp, float2(offset.x, offset.z) / resolution);
    float4 sample1 = tex.Sample(samp, float2(offset.y, offset.z) / resolution);
    float4 sample2 = tex.Sample(samp, float2(offset.x, offset.w) / resolution);
    float4 sample3 = tex.Sample(samp, float2(offset.y, offset.w) / resolution);

    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);

    return lerp(lerp(sample3, sample2, sx), lerp(sample1, sample0, sx), sy);
}
