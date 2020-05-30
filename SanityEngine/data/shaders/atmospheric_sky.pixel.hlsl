struct FullscreenVertexOutput {
    float4 position : SV_POSITION;
    float3 position_clipspace : CLIPPOS;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {};

#include "inc/atmospheric_scattering.hlsl"
#include "inc/standard_root_signature.hlsl"

float4 main(FullscreenVertexOutput input) : SV_TARGET {
    Camera camera = cameras[constants.camera_index];
    float4 position_clipspace = float4(input.position_clipspace, 1);
    float4 view_vector_worldspace = mul(mul(camera.inverse_view, camera.inverse_projection), position_clipspace);
    view_vector_worldspace /= view_vector_worldspace.w;

    Light sun = lights[0]; // Light 0 is always the sun
    float sun_strength = length(sun.color);

    float3 color = atmosphere(6471e3,
                              normalize(-view_vector_worldspace.xyz),
                              float3(0, 6371e3, 0),
                              -sun.direction,                   // direction of the sun
                              sun_strength,                     // intensity of the sun
                              6371e3,                           // radius of the planet in meters
                              6471e3,                           // radius of the atmosphere in meters
                              float3(5.5e-6, 13.0e-6, 22.4e-6), // Rayleigh scattering coefficient
                              21e-6,                            // Mie scattering coefficient
                              8e3,                              // Rayleigh scale height
                              1.2e3,                            // Mie scale height
                              0.758                             // Mie preferred scattering direction
    );

    float mu = dot(normalize(-view_vector_worldspace.xyz), -sun.direction);
    float mumu = mu * mu;
    float g = 0.9995;
    float gg = g * g;
    float phase_sun = 3.0 / (8.0 * PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg));
    float spot = smoothstep(0.0, 15.0, phase_sun) * sun_strength;

    return float4(color + spot, 1);
}
