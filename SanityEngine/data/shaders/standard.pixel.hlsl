struct VertexOutput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {};

#include "inc/standard_root_signature.hlsl"

float4 main(VertexOutput input) : SV_TARGET { 
    Light sun = lights[0];  // The sun is ALWAYS at index 0

    Camera camera = cameras[constants.camera_index];
    float4x4 clip_to_world = mul(camera.inverse_projection, camera.inverse_view);

    float4 position_worldspace = mul(camera.inverse_view, mul(camera.inverse_projection, input.position));

    /*RayDesc ray;
    ray.Origin = position_worldspace.xyz;
    ray.TMin = 0.1; // Slight offset so we don't self-intersect. TODO: Make this slope-scaled
    ray.Direction = -sun.direction; // Towards the sun
    ray.TMax = 1000;    // TODO: Pass this in with a CB

    // Shadow ray query
    RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;

    // Set up work
    q.TraceRayInline(raytracing_scene,
                     RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, ray);

    // Actually perform the trace
    q.Proceed();

    float3 light = float3(0.2f, 0.2f, 0.2f);
    if(q.CommittedStatus() != COMMITTED_TRIANGLE_HIT) {
        light = saturate(dot(input.normal, -sun.direction)) * sun.color;
    }*/

    // return float4(input.color.rgb * light, input.color.a);
    return float4(position_worldspace.xyz, 1);
}