#pragma once

#include "atmospheric_scattering.hlsl"
#include "standard_root_signature.hlsl"

float2 equi_uvs(float3 uvw) {
    float3 n = normalize(uvw), a = abs(n);
    float s = float(n.y > 0.0);
    float r = sqrt(1.0 - a.y) * (1.57073 + a.y * (-0.212114 + (0.074261 - 0.0187293 * a.y) * a.y));
    float t3 = min(a.z, a.x) / max(a.z, a.x);
    float t4 = t3 * t3, t5 = t4 * t4;
    t3 *= -0.0134805 * (-2.91598 + t4) * (6.7751 - 3.10708 * t4 + t5) * (3.75485 + 1.75931 * t4 + t5);
    t3 = a.x > a.z ? 1.570796327 - t3 : t3;
    t3 = n.z < 0.0 ? 3.141592654 - t3 : t3;
    return float2((t3 * sign(n.x) + 3.141592654) * 0.159154943, 1.0 - 0.31831 * r + (-1.0 + 0.63662 * r) * s);
}

float3 get_sky_in_direction(const in float3 view_vector_worldspace) {
    const float3 sky_view_vector = view_vector_worldspace * float3(1, -1, 1);
    uint skybox_index = per_frame_data[0].sky_texture_idx;
    if(skybox_index == 0) {
        const Light sun = lights[SUN_LIGHT_INDEX];
        const float sun_strength = length(sun.color);

        float3 direction_to_sun = normalize(-sun.direction_or_location);
        direction_to_sun.yz *= -1;

        return sun_and_atmosphere(direction_to_sun, sun_strength, sky_view_vector);

    } else {
        Texture2D skybox = textures[skybox_index];
        const float2 uvs = equi_uvs(normalize(sky_view_vector.xyz));
        return skybox.Sample(bilinear_sampler, uvs).rgb;
    }
}
