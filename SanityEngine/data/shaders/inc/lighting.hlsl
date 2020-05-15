// Unoptimized GGX specular term from http://filmicworlds.com/blog/optimizing-ggx-shaders-with-dotlh/

float g1_v(float ndotv, float k) { return 1.0f / (ndotv * (1.0f - k) + k); }

float3 ggx(float3 n, float3 v, float3 l, float roughness, float3 f0) {
    float alpha = roughness * roughness;

    float3 h = normalize(-v + l);

    float ndotl = saturate(dot(n, l));
    float ndotv = saturate(dot(n, v));
    float ndoth = saturate(dot(n, h));
    float ldoth = saturate(dot(l, h));

    // D
    float alpha_sqr = alpha * alpha;
    float pi = 3.141592;
    float denom = ndoth * ndoth * (alpha_sqr - 1.0) + 1.0f;
    float d = alpha_sqr / (pi * denom * denom);

    // F
    float3 f = f0 + (1.0 - f0) * pow(1.0f - ldoth, 5);

    // V
    float k = alpha / 2.0f;
    float vis = g1_v(ndotl, k) * g1_v(ndotv, k);

    return ndotl * d * f * vis;
}
