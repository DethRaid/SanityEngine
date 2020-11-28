#pragma once

RayDesc cast_shadow_ray(float3 start_location, float t_min, float3 direction, float t_max) {
    // Shadow ray query
    RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;

    RayDesc ray;
    ray.Origin = start_location;
    ray.TMin = t_min; // Slight offset so we don't self-intersect. TODO: Make this slope-scaled
    ray.Direction = direction;
    ray.TMax = t_max; // TODO: Pass this in with a CB

    // Set up work
    q.TraceRayInline(raytracing_scene,
                     RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
                     0xFF,
                     ray);

    // Actually perform the trace
    q.Proceed();
    if(q.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
        ray.TMax = q.CommittedRayT();

    } else {
        ray.TMax = 0;
    }

    return ray;
}
