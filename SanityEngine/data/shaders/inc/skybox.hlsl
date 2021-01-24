#pragma once

float2 equi_uvs(float3 uvw) {
    float3 n = normalize(uvw), a = abs(n);
    float s = float(n.y > 0.0);
    float r = sqrt(1.0 - a.y) * (1.57073 + a.y * (-0.212114 + (0.074261 - 0.0187293 * a.y) * a.y));
    float t3 = min(a.z, a.x) / max(a.z, a.x);
    float t4 = t3 * t3, t5 = t4 * t4;
    t3 *= -0.0134805 * (-2.91598 + t4) * (6.7751 - 3.10708 * t4 + t5) * (3.75485 + 1.75931 * t4 + t5);
    t3 = a.x > a.z ? 1.570796327 - t3 : t3;
    t3 = n.z < 0.0 ? 3.141592654 - t3 : t3;
    return float2((t3 * sign(n.x) + 3.141592654) * 0.159154943, 1.0 - 0.31831 * r + (-1.0 + 0.63662 * r) * s);
}
