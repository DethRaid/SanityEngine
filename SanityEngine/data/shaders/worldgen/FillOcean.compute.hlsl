struct TerrainParameters {
    float sea_level;
} constants;

Texture2D<float> heightmap : register(u0);

RWTexture2D<float> water_depth_map : register(u1);

[numthreads(8, 8, 1)] void main(uint3 dispatch_thread_id
                                : SV_DispatchThreadID) {
    uint2 texture_dimensions;
    heightmap.GetDimensions(texture_dimensions.x, texture_dimensions.y);

    int2 location = dispatch_thread_id.xy;
    if(!(location.x < texture_dimensions.x && location.y < texture_dimensions.y)) {
        return;
    }

    float2 uv = float2(location.x, location.y) / float2(texture_dimensions.x, texture_dimensions.y);

    float heightmap_sample = heightmap[uv];

    if(heightmap_sample < constants.sea_level) {
        water_depth_map[uv] = constants.sea_level - heightmap_sample;
    }
}