struct FullscreenVertexOutput {
    float4 position : SV_POSITION;
    float3 position_clipspace : CLIPPOS;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {};

#include "inc/atmospheric_scattering.hlsl"
#include "inc/sky.hlsl"
#include "inc/standard_root_signature.hlsl"

float4 main(FullscreenVertexOutput input) : SV_TARGET {
    const Camera camera = get_current_camera();
    const float4 location_clipspace = float4(input.position_clipspace, 1);
    float4 location_viewspace = mul(camera.inverse_projection, location_clipspace);
    location_viewspace /= location_viewspace.w;
    const float3 view_vector_worldspace = mul(camera.inverse_view, float4(location_viewspace.xyz, 0)).xyz;

    const float3 sky = get_sky_in_direction(view_vector_worldspace * float3(1, -1, 1));
    return float4(sky, 1);
}
