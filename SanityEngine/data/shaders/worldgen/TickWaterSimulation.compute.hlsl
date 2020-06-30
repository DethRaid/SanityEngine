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
    float our_height = heightmap[uv];
    float our_water_depth = water_depth_map[uv];

    if(our_water_depth > 0) {
        // If we already have water, there's no need to get water from our neighbors
        return;
    }

    bool should_have_water = true;

    // Check the 3x3 square of texels around us. If any one of them has a water depth of at least 1, and has a height higher than us, we get some water
    for(int v_offset = -1; v_offset < 2; v_offset ++) {
        for(int u_offset = -1; u_offset < 2; u_offset++) {
            float2 neighbor_uv = float2(location.x + u_offset, location.y + v_offset) / float2(texture_dimensions.x, texture_dimensions.y);
            float neighbor_height = heightmap[neighbor_uv];
            float neighbor_water_depth = water_depth_map[neighbor_uv];
            
            bool neighbor_has_water = neighbor_water_depth > 0;
            bool neighbor_is_above_us = neighbor_height > our_height;
            if(neighbor_has_water && neighbor_is_above_us) {
                should_have_water = true;

                // TODO: Is there a real performance benefit to exiting both the for loops here?
            }
        }
    }

    if(should_have_water) {
        // TODO: Figure out a good initial water depth
        water_depth_map[uv] = 0.5f;
    }
}