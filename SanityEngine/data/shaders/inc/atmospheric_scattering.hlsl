#pragma once

// Atmospheric scattering adapted from https://github.com/wwwtyro/glsl-atmosphere

#define PI 3.141592
#define iSteps 16
#define jSteps 8

float2 rsi(float3 r0, float3 rd, float sr) {
    // ray-sphere intersection that assumes
    // the sphere is centered at the origin.
    // No intersection when result.x > result.y
    float a = dot(rd, rd);
    float b = 2.0 * dot(rd, r0);
    float c = dot(r0, r0) - (sr * sr);
    float d = (b * b) - 4.0 * a * c;
    if(d < 0.0)
        return float2(1e5, -1e5);
    return float2((-b - sqrt(d)) / (2.0 * a), (-b + sqrt(d)) / (2.0 * a));
}

float3 atmosphere(float maxDepth,
                  float3 v,
                  float3 r0,
                  float3 pSun,
                  float iSun,
                  float rPlanet,
                  float rAtmos,
                  float3 kRlh,
                  float kMie,
                  float shRlh,
                  float shMie,
                  float g) {
    // Normalize the sun and view directions.
    pSun = normalize(pSun);
    v = normalize(v);

    // Calculate the step size of the primary ray.
    float2 p = rsi(r0, v, rAtmos);

    if(p.x > p.y)
        return float3(0, 0, 0);
    p.y = min(p.y, rsi(r0, v, rPlanet).x);

    float rayLength = p.y - p.x;
    rayLength = min(rayLength, maxDepth); // limit marching to scene depth in order to avoid "transparent" geometry
    float iStepSize = rayLength / float(iSteps);

    // Initialize the primary ray time.
    float iTime = 0.0;

    // Initialize accumulators for Rayleigh and Mie scattering.
    float3 totalRlh = float3(0, 0, 0);
    float3 totalMie = float3(0, 0, 0);

    // Initialize optical depth accumulators for the primary ray.
    float iOdRlh = 0.0;
    float iOdMie = 0.0;

    // Calculate the Rayleigh and Mie phases.
    float mu = dot(v, pSun);
    float mumu = mu * mu;
    float gg = g * g;
    float pRlh = 3.0 / (16.0 * PI) * (1.0 + mumu);
    float pMie = 3.0 / (8.0 * PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg));

    // Sample the primary ray.
    // float3 failAccum = float3(0.0f, 0.0f, 0.0f);
    for(int i = 0; i < iSteps; i++) {

        // Calculate the primary ray sample position.
        float3 iPos = r0 + v * (iTime + iStepSize * 0.5);

        // Increment the primary ray time.
        iTime += iStepSize;

        // shadow this step if we can't be seen from the sun
        // i.e. "shadow map" the ray on this position
        // --------------------
        // if(PointInShadow(iPos - float3(0.0f, 6371e3f, 0.0f))) {
        //     continue;
        // }

        // Calculate the height of the sample.
        float iHeight = length(iPos) - rPlanet;

        // Calculate the optical depth of the Rayleigh and Mie scattering for this step.
        float odStepRlh = exp(-iHeight / shRlh) * iStepSize;
        float odStepMie = exp(-iHeight / shMie) * iStepSize;

        // Accumulate optical depth.
        iOdRlh += odStepRlh;
        iOdMie += odStepMie;

        // Calculate the step size of the secondary ray.
        float jStepSize = rsi(iPos, pSun, rAtmos).y / float(jSteps);

        // Initialize the secondary ray time.
        float jTime = 0.0;

        // Initialize optical depth accumulators for the secondary ray.
        float jOdRlh = 0.0;
        float jOdMie = 0.0;

        // Sample the secondary ray.
        for(int j = 0; j < jSteps; j++) {

            // Calculate the secondary ray sample position.
            float3 jPos = iPos + pSun * (jTime + jStepSize * 0.5);

            // Calculate the height of the sample.
            float jHeight = length(jPos) - rPlanet;

            // Accumulate the optical depth.
            jOdRlh += exp(-jHeight / shRlh) * jStepSize;
            jOdMie += exp(-jHeight / shMie) * jStepSize;

            // Increment the secondary ray time.
            jTime += jStepSize;
        }

        // Calculate attenuation.
        float3 attn = exp(-(kMie * (iOdMie + jOdMie) + kRlh * (iOdRlh + jOdRlh)));

        // Accumulate scattering.
        totalRlh += odStepRlh * attn;
        totalMie += odStepMie * attn;
    }

    // Calculate and return the final color.
    float3 atmos = iSun * (pRlh * kRlh * totalRlh + pMie * kMie * totalMie);

    if(!any(isnan(atmos))) {
        return atmos * PI;
    }

    return 0.0;
}

float3 sun_and_atmosphere(in float3 direction_to_sun, const in float sun_strength, const in float3 view_vector_worldspace) {
    const float3 color = atmosphere(6471e3,
                                    view_vector_worldspace.xyz,
                                    float3(0, 6371e3, 0),
                                    direction_to_sun,                 // direction of the sun
                                    sun_strength,                     // intensity of the sun
                                    6371e3,                           // radius of the planet in meters
                                    6471e3,                           // radius of the atmosphere in meters
                                    float3(5.5e-6, 13.0e-6, 22.4e-6), // Rayleigh scattering coefficient
                                    21e-6,                            // Mie scattering coefficient
                                    8e3,                              // Rayleigh scale height
                                    1.2e3,                            // Mie scale height
                                    0.758                             // Mie preferred scattering direction
    );

    const float mu = dot(normalize(view_vector_worldspace.xyz), direction_to_sun);
    const float mumu = mu * mu;
    const float g = 0.9995;
    const float gg = g * g;
    const float phase_sun = 3.0 / (8.0 * PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg));
    const float spot = smoothstep(0.0, 15.0, phase_sun) * sun_strength;

    const float3 atmosphere = color + spot;
    if(any(isnan(atmosphere))) {
        return 0.f;
    }

    if(any(atmosphere < 0.f)) {
        return 0.f;
    }

    return atmosphere;
}
