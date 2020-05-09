// Converts a silly BVE texture into a texture with proper transparency

struct TransparencyConstants {
    /*!
     * \brief Sentinel value to indicate transparent texels
     */
    float3 decal_transparent_color;
} constants;

RWTexture2D<float4> texture : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dispatch_thread_id : SV_DISPATCHTHREADID) {
    float4 texel = texture[dispatch_thread_id.xy];
    if(dot(texel.rgb, constants.decal_transparent_color) > 0.99) {

    }
}
