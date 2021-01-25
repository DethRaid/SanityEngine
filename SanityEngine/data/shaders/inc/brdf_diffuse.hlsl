#pragma once

// BRDF from https://google.github.io/filament/Filament.html#listing_glslbrdf

float D_GGX(float NoH, float a) {
    float a2 = a * a;
    float f = (NoH * a2 - NoH) * NoH + 1.0;
    return a2 / (PI * f * f);
}

float3 F_Schlick(float u, float3 f0, float3 f90) { return f0 + (f90 - f0) * pow(1.0 - u, 5.0); }

float V_SmithGGXCorrelated(float NoV, float NoL, float a) {
    float a2 = a * a;
    float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
    float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
    return 0.5 / (GGXV + GGXL);
}

float3 Fr_CookTorance(float3 n, float roughness, float f0, float3 l, float3 v) {
    float3 h = normalize(v + l);

    float NoV = abs(dot(n, v)) + 1e-5;
    float NoL = clamp(dot(n, l), 0.0, 1.0);
    float NoH = clamp(dot(n, h), 0.0, 1.0);
    float LoH = clamp(dot(l, h), 0.0, 1.0);
	
    float D = D_GGX(NoH, roughness);
    float3 F = F_Schlick(LoH, f0, 1);
    float V = V_SmithGGXCorrelated(NoV, NoL, roughness);

	return (D * V) * F;
}

float Fd_Lambert(float3 n, float3 l) { return saturate(dot(n, l)); }

float3 brdf(float3 base_color, float3 n, float metalness, float perceptualRoughness, float3 l, float3 v) {
	// Remapping from https://google.github.io/filament/Filament.html#materialsystem/parameterization/remapping
    const float3 albedo = (1.0 - metalness) * base_color;
    float roughness = perceptualRoughness * perceptualRoughness;
	
    const float dielectric_f0 = 0.035;
    const float3 f0 = lerp(float3(dielectric_f0, dielectric_f0, dielectric_f0), base_color, metalness);
    
    // specular BRDF
    float3 Fr = Fr_CookTorance(n, roughness, f0, l, v);

    // diffuse BRDF
    float3 Fd = albedo * Fd_Lambert(n, l);

    return Fd;
    //Fr + Fd;
}
