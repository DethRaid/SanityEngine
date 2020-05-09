// Converts a silly BVE texture into a texture with proper transparency

struct TransparencyConstants {
    /*!
     * \brief Sentinel value to indicate transparent texels
     */
    float3 decal_transparent_color;
} constants;

Texture2D<float4> input_texture : register(t0);

RWTexture2D<float4> output_texture : register(u0);

float4 load_gamma(int2 position) {
    float4 srgb = input_texture[position] / 255.0f;
    return float4(pow(srgb.rgb, 2.2f), srgb.a);
}

void store_gamma(int2 location, float4 value) {
    float4 srgb = float4(pow(value.rgb, 1.0f / 2.2f), value.a);
    output_texture[location] = uint4(srgb * 255);
}

float4 load_strip_blue(int2 location) {
    float4 value = load_gamma(location);
    if(dot(value.xyz, constants.decal_transparent_color) > 0.99) {
        value = 0.0;
    }
    return value;
}

float4 conditional_load(int2 location, uint2 texture_dimensions) {
    if(0 <= location.x && 0 <= location.y && location.x < texture_dimensions.x && location.y < texture_dimensions.y) {
        float4 value = load_strip_blue(location);
        if (value.a == 0.0) {
            value = 0.0;
        } else {
            value.a = 1.0;
        }
        return value;
    } else {
        return 0.0;
    }
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id : SV_DISPATCHTHREADID) {
    uint2 texture_dimensions;
    input_texture.GetDimensions(texture_dimensions.x, texture_dimensions.y);    
    
    int2 location = dispatch_thread_id.xy;
    if (!(location.x < texture_dimensions.x && location.y < texture_dimensions.y)) {
        return;
    }
    float4 texel11 = load_strip_blue(location);
    if (texel11.w == 0.0) {
        float4 texel00 = conditional_load(location + int2(-1, -1), texture_dimensions);
        float4 texel10 = conditional_load(location + int2(0, -1), texture_dimensions);
        float4 texel20 = conditional_load(location + int2(1, -1), texture_dimensions);
        float4 texel01 = conditional_load(location + int2(-1, 0), texture_dimensions);
        float4 texel21 = conditional_load(location + int2(1, 0), texture_dimensions);
        float4 texel02 = conditional_load(location + int2(-1, 1), texture_dimensions);
        float4 texel12 = conditional_load(location + int2(0, 1), texture_dimensions);
        float4 texel22 = conditional_load(location + int2(1, 1), texture_dimensions);
        
        float4 sum = texel00 + texel01 + texel02 + texel10 + texel12 + texel20 + texel21 + texel22;
        float scale = sum.w;
        float3 average = sum.xyz / scale;
        store_gamma(location, float4(average, 0.0));
    } else {
        store_gamma(location, texel11);
    }
}
