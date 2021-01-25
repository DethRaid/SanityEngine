#pragma once

// BRDF from https://google.github.io/filament/Filament.html#listing_glslbrdf

float d_ggx(const float NoH, const float a) {
    // ReSharper disable CppInconsistentNaming
    const float a2 = a * a;
    const float f = (NoH * a2 - NoH) * NoH + 1.0;
    return a2 / (PI * f * f);
    // ReSharper restore CppInconsistentNaming
}

float3 fresnel_schlick(const float u, const float3 f0, const float3 f90) { return f0 + (f90 - f0) * pow(1.0 - u, 5.0); }

float smith_ggx_correlated_v(const float NoV, const float NoL, const float a) {
    // ReSharper disable CppInconsistentNaming
    const float a2 = a * a;
    const float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
    const float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
    return 0.5 / (GGXV + GGXL);
    // ReSharper restore CppInconsistentNaming
}

float3 cook_torrance(const float3 n, const float roughness, const float3 f0, const float3 l, const float3 v) {
    // ReSharper disable CppInconsistentNaming
    const float3 h = normalize(v + l);

    const float NoV = abs(dot(n, v)) + 1e-5;
    const float NoL = saturate(dot(n, l));
    const float NoH = saturate(dot(n, h));
    const float LoH = saturate(dot(l, h));

    const float D = d_ggx(NoH, roughness);
    const float3 F = fresnel_schlick(LoH, f0, 1);
    const float V = smith_ggx_correlated_v(NoV, NoL, roughness);

    return (D * V) * F;
    // ReSharper restore CppInconsistentNaming
}

float lambert(const float3 n, const float3 l) { return saturate(dot(n, l)); }

float3 brdf(const SurfaceInfo surface, const float3 l, const float3 v) {
    // Remapping from https://google.github.io/filament/Filament.html#materialsystem/parameterization/remapping
    const float3 albedo = (1.0 - surface.metalness) * surface.base_color.rgb;
    const float roughness = surface.roughness * surface.roughness;

    const float dielectric_f0 = 0.035; // TODO: Get this from a texture
    const float3 f0 = lerp(dielectric_f0.xxx, surface.base_color.rgb, surface.metalness);

    const float3 specular = cook_torrance(surface.normal, roughness, f0, l, v);

    const float3 diffuse = albedo * lambert(surface.normal, l);
    return diffuse;

    return specular + diffuse;
}
