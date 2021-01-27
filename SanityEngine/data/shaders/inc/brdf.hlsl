#pragma once

float D_GGX(float NoH, float roughness) {
    float k = roughness / (1.0 - NoH * NoH + roughness * roughness);
    return k * k * (1.0 / PI);
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

float3 brdf(in SurfaceInfo surface, const float3 l, const float3 v) {
    surface.normal *= -1;
	
    // Remapping from https://google.github.io/filament/Filament.html#materialsystem/parameterization/remapping
    const float3 albedo = (1.0 - surface.metalness) * surface.base_color.rgb;

    const float dielectric_f0 = 0.035; // TODO: Get this from a texture
    const float3 f0 = lerp(dielectric_f0.xxx, surface.base_color.rgb, surface.metalness);

    float3 h = normalize(v + l);

    float NoV = abs(dot(surface.normal, v)) + 1e-5;
    float NoL = saturate(dot(surface.normal, l));
    float NoH = saturate(dot(surface.normal, h));
    float VoH = saturate(dot(v, h));

    // perceptually linear roughness to roughness (see parameterization)
    float roughness = surface.roughness * surface.roughness;

    float D = D_GGX(NoH, roughness);
    float3 F = F_Schlick(VoH, f0, 1.f);
    float V = V_SmithGGXCorrelated(NoV, NoL, roughness);

    // specular BRDF
    float3 Fr = (D * V) * F;
    return Fr;

    // diffuse BRDF
    float3 Fd = NoL * Fd_Lambert() /* Fd_Burley(NoV, NoL, LoH, roughness) */ * albedo;

    return Fd + Fr;
}
