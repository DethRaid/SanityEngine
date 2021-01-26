#pragma once

float D_GGX(float NoH, float a) {
    float a2 = a * a;
    float f = (NoH * a2 - NoH) * NoH + 1.0;
    return a2 / (PI * f * f);
}

float3 F_Schlick(float u, float3 f0, float f90) { return f0 + (f90 - f0) * pow(1.0 - u, 5.0); }

float V_SmithGGXCorrelated(float NoV, float NoL, float a) {
    float a2 = a * a;
    float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
    float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
    return 0.5 / (GGXV + GGXL);
}

float Fd_Lambert() { return 1.0 / PI; }

float Fd_Burley(float NoV, float NoL, float LoH, float roughness) {
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    float lightScatter = F_Schlick(NoL, 1.0, f90);
    float viewScatter = F_Schlick(NoV, 1.0, f90);
    return lightScatter * viewScatter * (1.0 / PI);
}

float3 brdf(const SurfaceInfo surface, const float3 l, const float3 v) {
    // Remapping from https://google.github.io/filament/Filament.html#materialsystem/parameterization/remapping
    const float3 albedo = (1.0 - surface.metalness) * surface.base_color.rgb;

    const float dielectric_f0 = 0.035; // TODO: Get this from a texture
    const float3 f0 = lerp(dielectric_f0.xxx, surface.base_color.rgb, surface.metalness);

    float3 h = normalize(v + l);

    float NoV = abs(dot(surface.normal, v)) + 1e-5;
    float NoL = clamp(dot(surface.normal, l), 0.0, 1.0);
    float NoH = clamp(dot(surface.normal, h), 0.0, 1.0);
    float LoH = clamp(dot(l, h), 0.0, 1.0);

    // perceptually linear roughness to roughness (see parameterization)
    float roughness = surface.roughness * surface.roughness;

    float D = D_GGX(NoH, NoH * roughness);
    float3 F = F_Schlick(LoH, f0, 1.f);
    float V = V_SmithGGXCorrelated(NoV, NoL, roughness);

    // specular BRDF
    float3 Fr = (D * V) * F;

    // diffuse BRDF
    float3 Fd = NoL * Fd_Lambert() * albedo;

    return Fd + Fr;
}
