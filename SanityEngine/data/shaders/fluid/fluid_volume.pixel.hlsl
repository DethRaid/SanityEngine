
#include "fluid_sim.hpp"
#include "inc/standard_root_signature.hlsl"

struct VertexShaderOutput {
    float4 location_ndc : SV_POSITION;
    float3 location_worldspace : WORLDPOS;
    float3 normal_worldspace : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

struct Ray {
    float3 origin;
    float3 direction;
};

struct AABB {
    float3 min;
    float3 max;
};

// find intersection points of a ray with a box
bool intersectBox(Ray r, AABB aabb, out float t0, out float t1) {
    float3 invR = 1.0 / r.direction;
    float3 tbot = invR * (aabb.min - r.origin);
    float3 ttop = invR * (aabb.max - r.origin);
    float3 tmin = min(ttop, tbot);
    float3 tmax = max(ttop, tbot);
    float2 t = max(tmin.xx, tmin.yz);
    t0 = max(t.x, t.y);
    t = min(tmax.xx, tmax.yz);
    t1 = min(t.x, t.y);
    return t0 <= t1;
}

float4 main(in VertexShaderOutput input) : SV_TARGET {
    const FrameConstants frame_constants = get_frame_constants();
    const Camera view_camera = get_current_camera();
    GET_CURRENT_DATA(GpuFluidVolumeState, fluid_volume);

    // Step through the volume, sampling the texture as needed
    const float3 start_pos_worldspace = input.location_worldspace;

    Ray ray;
    ray.origin = -view_camera.view[4].xyz;
    ray.direction = normalize(ray.origin - input.location_worldspace);

    return float4(0.f, 0.f, 0.f, 0.f);
}
