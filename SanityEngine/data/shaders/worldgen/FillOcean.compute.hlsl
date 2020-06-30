struct TerrainParameters {
    float sea_level;
} constants;

RWTexture2D<float4> heightmap : register(u0);

[numthreads(8, 8, 1)] void main(uint3 dispatch_thread_id
                                : SV_DispatchThreadID) {
    uint2 texture_dimensions;
    heightmap.GetDimensions(texture_dimensions.x, texture_dimensions.y);

    int2 location = dispatch_thread_id.xy;
    if(!(location.x < texture_dimensions.x && location.y < texture_dimensions.y)) {
        return;
    }

    float2 uv = location / texture_dimensions;

    float4 heightmap_sample = heightmap[uv];

    if(heightmap_sample.x < constants.sea_level) {
        heightmap[uv] = float4(heightmap_sample.x, constants.sea_level - heightmap_sample.x, heightmap_sample.zw);

    } else {
        heightmap[uv] = float4(heightmap_sample.x, 0, heightmap_sample.zw);
    }
}