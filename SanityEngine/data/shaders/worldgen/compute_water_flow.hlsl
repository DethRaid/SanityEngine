Texture2D<float> heightmap : register(t0);

Texture2D<float> input_watermap : register(t1);

RWTexture2D<float> output_watermap : register(t2);

[numthreads(8, 8, 1)] void main(uint3 dispatch_thread_id
                                : SV_DispatchThreadID) {
    uint2 dimensions;
    heightmap.GetDimensions(dimensions.x, dimensions.y);
    if(dispatch_thread_id.x >= dimensions.x || dispatch_thread_id.y >= dimensions.y) {
        // Don't operate outside the bounds of the textures
        return;
    }

    float initial_water_amount = input_watermap[dispatch_thread_id.xy];

}