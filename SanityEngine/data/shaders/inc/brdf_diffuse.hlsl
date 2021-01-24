#pragma once

float3 lambert_diffuse(float3 n, float3 l) {
    const float ndotl = saturate(dot(n, l));
    return ndotl * 1.05;
}

float sqr(float x) { return x * x; }

// Burley Diffuse from https://github.com/wdas/brdf/blob/main/src/brdfs/disney.brdf

float SchlickFresnel(float u) {
    float m = clamp(1 - u, 0, 1);
    float m2 = m * m;
    return m2 * m2 * m; // pow(m,5)
}

float3 mon2lin(float3 x) { return float3(pow(x[0], 2.2), pow(x[1], 2.2), pow(x[2], 2.2)); }

float3 burley_diffuse(float3 N, float3 L, float3 V, float3 albedo, float roughness) {
    float NdotL = dot(N, L);
    float NdotV = dot(N, V);
    if(NdotL < 0 || NdotV < 0) {
        return float3(0, 0, 0);
    }

    float3 H = normalize(L + V);
    float LdotH = dot(L, H);

    float3 Cdlin = mon2lin(albedo);

    // Diffuse fresnel - go from 1 at normal incidence to .5 at grazing
    // and mix in diffuse retro-reflection based on roughness
    float FL = SchlickFresnel(NdotL), FV = SchlickFresnel(NdotV);
    float Fd90 = 0.5 + 2 * LdotH * LdotH * roughness;
    float Fd = lerp(1.0, Fd90, FL) * lerp(1.0, Fd90, FV);

    return (1 / PI) * Fd * Cdlin;
}

// Unoptimized GGX specular term from http://filmicworlds.com/blog/optimizing-ggx-shaders-with-dotlh/

float3 fresnel(float3 f0, float ldoth) { return f0 + (1.0 - f0) * pow(1.0f - ldoth, 5); }

float G1V(float dotNV, float k) { return 1.0f / (dotNV * (1.0f - k) + k); }

float GGX(float3 V, float3 L, float3 N, float roughness, float F0) {
    float alpha = roughness * roughness;

    float3 H = normalize(V + L);

    float dotNL = saturate(dot(N, L));
    float dotNV = saturate(dot(N, V));
    float dotNH = saturate(dot(N, H));
    float dotLH = saturate(dot(L, H));

    float F, D, vis;

    // D
    float alphaSqr = alpha * alpha;
    float pi = 3.14159f;
    float denom = dotNH * dotNH * (alphaSqr - 1.0) + 1.0f;
    D = alphaSqr / (pi * denom * denom);

    // F
    float dotLH5 = pow(1.0f - dotLH, 5);
    F = F0 + (1.0 - F0) * (dotLH5);

    // V
    float k = alpha / 2.0f;
    vis = G1V(dotNL, k) * G1V(dotNV, k);

    float specular = dotNL * D * F * vis;
    return specular;
}

float3 brdf_diffuse(float3 albedo, float3 f0, float roughness, float3 normal, float3 light_vector, float3 eye_vector) {
    // const float3 h = normalize(-eye_vector + light_vector);
    // const float ldoth = saturate(dot(light_vector, h));
    // const float3 f = fresnel(f0, ldoth);
    // const float3 diffuse = lambert_diffuse(normal, light_vector) * albedo * (float3(1.0f, 1.0f, 1.0f) - f);

    const float3 diffuse = burley_diffuse(normal, light_vector, eye_vector, albedo, roughness);

    return diffuse;
}

float3 brdf_specular(float3 f0, float roughness, float3 normal, float3 light_vector, float3 eye_vector) {
    const float3 h = normalize(-eye_vector + light_vector);
    const float ldoth = saturate(dot(light_vector, h));
    const float3 f = fresnel(f0, ldoth);

    const float3 specular = GGX(eye_vector, light_vector, normal, roughness, 0);

    return specular * f;
}
