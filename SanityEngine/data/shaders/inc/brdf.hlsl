// ReSharper disable CppInconsistentNaming
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

float3 Fd_Burley(float NoV, float NoL, float LoH, float roughness) {
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    float3 lightScatter = F_Schlick(NoL, 1.0, f90);
    float3 viewScatter = F_Schlick(NoV, 1.0, f90);

    return lightScatter * viewScatter * (1.0 / PI);
}

float PDF_GGX(const in float roughness, const in float3 n, const in float3 l, const in float3 v) {
    float3 h = normalize(v + l);

    const float VoH = saturate(dot(v, h));
    const float NoH = saturate(dot(n, h));

    // D_GGX(NoH, roughness) * NoH / (4.0 + VoH);

    return 1 / (4 * VoH);
}

float3 brdf(in SurfaceInfo surface, float3 l, const float3 v) {
    // Remapping from https://google.github.io/filament/Filament.html#materialsystem/parameterization/remapping
    const float dielectric_f0 = 0.04; // TODO: Get this from a texture
    const float3 f0 = lerp(dielectric_f0.xxx, surface.base_color.rgb, surface.metalness);

    const float3 diffuse_color = surface.base_color.rgb * (1 - dielectric_f0) * (1 - surface.metalness);

	// l.x *= -1;

    const float3 h = normalize(v + l);

    float NoV = dot(surface.normal, v) + 1e-5;
    float NoL = dot(surface.normal, l);
    const float NoH = saturate(dot(surface.normal, h));
    const float VoH = saturate(dot(v, h));

    if(NoL < 0) {
        return 0;
    }

    NoV = abs(NoV);
    NoL = saturate(NoL);
    
    const float D = D_GGX(NoH, surface.roughness);
    const float3 F = F_Schlick(VoH, f0, 1.f);
    const float V = V_SmithGGXCorrelated(NoV, NoL, surface.roughness);

    // specular BRDF
    const float3 Fr = (D * V) * F;

    // diffuse BRDF
    const float LoH = saturate(dot(l, h));
    const float3 Fd = diffuse_color * Fd_Burley(NoV, NoL, LoH, surface.roughness);

    return Fd + Fr;
}

float3 brdf_single_ray(in SurfaceInfo surface, const float3 l, const float3 v) {
    // Remapping from https://google.github.io/filament/Filament.html#materialsystem/parameterization/remapping
    const float dielectric_f0 = 0.04; // TODO: Get this from a texture
    const float3 f0 = lerp(dielectric_f0.xxx, surface.base_color.rgb, surface.metalness);

    const float3 diffuse_color = surface.base_color.rgb * (1 - dielectric_f0) * (1 - surface.metalness);

    float3 h = normalize(v + l);

    float NoV = dot(surface.normal, v) + 1e-5;
    float NoL = dot(surface.normal, l);
    float NoH = saturate(dot(surface.normal, h));
    float VoH = saturate(dot(v, h));

    if(NoV < 0 || NoL < 0) {
        return 0;
    }

    NoV = abs(NoV);
    NoL = saturate(NoL);
    
    float3 F = F_Schlick(VoH, f0, 1.f);
    float V = V_SmithGGXCorrelated(NoV, NoL, surface.roughness);

    // specular BRDF
    float3 Fr = V * F;

    // diffuse BRDF
    const float LoH = saturate(dot(l, h));
    float3 Fd = diffuse_color * Fd_Burley(NoV, NoL, LoH, surface.roughness);

    return Fd + Fr;
}
