
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

    float3 at(float t);
};

float3 Ray::at(const float t) { return origin + direction * t; }

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

#define NUM_STEPS 8

float4 main(in VertexShaderOutput input) : SV_TARGET {
    const FrameConstants frame_constants = get_frame_constants();
    const Camera view_camera = get_current_camera();
    const float4x4 model_matrix = get_current_model_matrix();
    GET_CURRENT_DATA(GpuFluidVolumeState, fluid_volume);

    Texture3D density_tex = textures3d[fluid_volume.density_textures[0]];

    // Step through the volume, sampling the texture as needed
    const float3 start_pos_worldspace = input.location_worldspace;

    Ray ray;
    ray.origin = -view_camera.view[4].xyz;
    ray.direction = normalize(ray.origin - input.location_worldspace);

    AABB box;

    // The origin of a fluid volume is at the bottom of that volume
    box.min = model_matrix[3].xyz - float3(fluid_volume.size.x, 0.f, fluid_volume.size.y) / 2.f;
    box.max = model_matrix[3].xyz + fluid_volume.size.xyz * float3(0.5f, 1.f, .5f);

    float t0, t1;
    bool intersects = intersectBox(ray, box, t0, t1);
    if(!intersects) {
        return float4(0.f, 0.f, 0.f, 0.f);
    }

    const float3 p0 = ray.at(t0);
    const float3 p1 = ray.at(t1);
    const float3 ray_segment = p1 - p0;
    const float3 ray_step = ray_segment / NUM_STEPS;

    float3 cur_pos = float3(0, 0, 0);
    float density = 0;
    for(int i = 0; i < NUM_STEPS; i++) {
        density += density_tex.Sample(bilinear_sampler, cur_pos).r;
        cur_pos += ray_step;
    }

    density /= NUM_STEPS;

    return float4(density.xxx, 1.f);
}
