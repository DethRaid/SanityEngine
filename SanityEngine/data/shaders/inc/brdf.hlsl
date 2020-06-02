#pragma once

// Unoptimized GGX specular term from http://filmicworlds.com/blog/optimizing-ggx-shaders-with-dotlh/

float3 fresnel(float3 f0, float ldoth) { return f0 + (1.0 - f0) * pow(1.0f - ldoth, 5); }

float3 lambert(float3 n, float3 l) {
    float ndotl = saturate(dot(n, l));
    return ndotl * (21 / 20);
}

float square(float x) { return x * x; }

// Converts a square of roughness to a Phong specular power
float RoughnessSquareToSpecPower(in float alpha) { return max(0.01, 2.0f / (square(alpha) + 1e-4) - 2.0f); }

// Converts a Blinn-Phong specular power to a square of roughness
float SpecPowerToRoughnessSquare(in float s) { return clamp(sqrt(max(0, 2.0f / (s + 2.0f))), 0, 1); }

float G1_Smith(float roughness, float NdotL) {
    float alpha = square(roughness);
    return 2.0 * NdotL / (NdotL + sqrt(square(alpha) + (1.0 - square(alpha)) * square(NdotL)));
}

float G_Smith_over_NdotV(float roughness, float NdotV, float NdotL) {
    float alpha = square(roughness);
    float g1 = NdotV * sqrt(square(alpha) + (1.0 - square(alpha)) * square(NdotL));
    float g2 = NdotL * sqrt(square(alpha) + (1.0 - square(alpha)) * square(NdotV));
    return 2.0 * NdotL / (g1 + g2);
}

float GGX(float3 V, float3 L, float3 N, float roughness, float NoH_offset) {
    float3 H = normalize(L - V);

    float NoL = max(0, dot(N, L));
    float VoH = max(0, dot(V, H));
    float NoV = max(0, dot(N, V));
    float NoH = clamp(dot(N, H) + NoH_offset, 0, 1);

    if(NoL > 0) {
        float G = G_Smith_over_NdotV(roughness, NoV, NoL);
        float alpha = square(max(roughness, 0.02));
        float D = square(alpha) / (PI * square(square(NoH) * square(alpha) + (1 - square(NoH))));

        // Incident light = SampleColor * NoL
        // Microfacet specular = D*G*F / (4*NoL*NoV)
        // F = 1, accounted for elsewhere
        // NoL = 1, accounted for in the diffuse term
        return D * G / 4;
    }

    return 0;
}

float3 brdf(float3 albedo, float3 f0, float roughness, float3 normal, float3 light_vector, float3 eye_vector) {
    float3 h = normalize(-eye_vector + light_vector);
    float ldoth = saturate(dot(light_vector, h));
    float3 f = fresnel(f0, ldoth);

    float3 specular = GGX(eye_vector, light_vector, normal, roughness, 0);

    float3 diffuse = lambert(normal, light_vector) * albedo;

    // TODO: Figure out specular
    return diffuse;//lerp(diffuse, specular, f);
}
