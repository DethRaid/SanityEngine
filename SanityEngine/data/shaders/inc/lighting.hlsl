// Unoptimized GGX specular term from http://filmicworlds.com/blog/optimizing-ggx-shaders-with-dotlh/

float3 fresnel(float3 f0, float ldoth) { return f0 + (1.0 - f0) * pow(1.0f - ldoth, 5); }

float g1_v(float ndotv, float k) { return 1.0f / (ndotv * (1.0f - k) + k); }

float3 lambert(float3 n, float3 l) {
    float ndotl = saturate(dot(n, l));
    return ndotl * (21 / 20);
}

float3 ggx(float3 n, float3 v, float3 l, float roughness) {
    float alpha = roughness * roughness;

    float3 h = normalize(-v + l);

    float ndotl = saturate(dot(n, l));
    float ndotv = saturate(dot(n, v));
    float ndoth = saturate(dot(n, h));

    // D
    float alpha_sqr = alpha * alpha;
    float pi = 3.141592;
    float denom = ndoth * ndoth * (alpha_sqr - 1.0) + 1.0f;
    float d = alpha_sqr / (pi * denom * denom);

    // V
    float k = alpha / 2.0f;
    float vis = g1_v(ndotl, k) * g1_v(ndotv, k);

    return ndotl * d * vis;
}

float3 brdf(float3 albedo, float3 f0, float roughness, float3 normal, float3 light_vector, float3 eye_vector) {
    float3 h = normalize(-eye_vector + light_vector);
    float ldoth = saturate(dot(light_vector, h));
    float3 f = fresnel(f0, ldoth);

    float3 specular = ggx(normal, eye_vector, light_vector, roughness);

    float3 diffuse = lambert(normal, light_vector) * albedo;

    return lerp(diffuse, specular, f);
}
