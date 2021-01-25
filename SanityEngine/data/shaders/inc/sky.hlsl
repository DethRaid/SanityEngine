#pragma once

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
    uint skybox_index = per_frame_data[0].sky_texture_idx;
    if(skybox_index == 0) {
        const Light sun = lights[0]; // Light 0 is always the sun
        const float sun_strength = length(sun.color);

        float3 light_direction = normalize(-sun.direction);
        light_direction.y *= -1;

        const float3 color = atmosphere(6471e3,
                                        normalize(view_vector_worldspace.xyz),
                                        float3(0, 6371e3, 0),
                                        light_direction,                  // direction of the sun
                                        sun_strength,                     // intensity of the sun
                                        6371e3,                           // radius of the planet in meters
                                        6471e3,                           // radius of the atmosphere in meters
                                        float3(5.5e-6, 13.0e-6, 22.4e-6), // Rayleigh scattering coefficient
                                        21e-6,                            // Mie scattering coefficient
                                        8e3,                              // Rayleigh scale height
                                        1.2e3,                            // Mie scale height
                                        0.758                             // Mie preferred scattering direction
        );

        const float mu = dot(normalize(view_vector_worldspace.xyz), light_direction);
        const float mumu = mu * mu;
        const float g = 0.9995;
        const float gg = g * g;
        const float phase_sun = 3.0 / (8.0 * PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg));
        const float spot = smoothstep(0.0, 15.0, phase_sun) * sun_strength;

        return color + spot;

    } else {
        Texture2D skybox = textures[skybox_index];
        const float2 uvs = equi_uvs(normalize(view_vector_worldspace.xyz));
        return skybox.Sample(bilinear_sampler, uvs).rgb;
    }
}