struct FullscreenVertexOutput {
    float4 position : SV_POSITION;
    float3 position_clipspace : CLIPPOS;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {};

#include "inc/atmospheric_scattering.hlsl"
#include "inc/skybox.hlsl"
#include "inc/standard_root_signature.hlsl"

float4 main(FullscreenVertexOutput input) : SV_TARGET {
    const Camera camera = cameras[constants.camera_index];
    const float4 location_clipspace = float4(input.position_clipspace, 1);
    float4 location_viewspace = mul(camera.inverse_projection, location_clipspace);
    location_viewspace /= location_viewspace.w;
    const float4 view_vector_worldspace = mul(camera.inverse_view, float4(location_viewspace.xyz, 0));

    uint skybox_index = per_frame_data[0].sky_texture_idx;
    if(skybox_index == 0) {
        const Light sun = lights[0]; // Light 0 is always the sun
        const float sun_strength = length(sun.color);

        float3 light_direction = normalize(-sun.direction);
        light_direction.y *= -1;

        const float3 color = atmosphere(6471e3,
                                        normalize(view_vector_worldspace.xyz),
                                        float3(0, 6371e3, 0),
                                        light_direction,                  // direction of the sun
                                        sun_strength,                     // intensity of the sun
                                        6371e3,                           // radius of the planet in meters
                                        6471e3,                           // radius of the atmosphere in meters
                                        float3(5.5e-6, 13.0e-6, 22.4e-6), // Rayleigh scattering coefficient
                                        21e-6,                            // Mie scattering coefficient
                                        8e3,                              // Rayleigh scale height
                                        1.2e3,                            // Mie scale height
                                        0.758                             // Mie preferred scattering direction
        );

        const float mu = dot(normalize(view_vector_worldspace.xyz), light_direction);
        const float mumu = mu * mu;
        const float g = 0.9995;
        const float gg = g * g;
        const float phase_sun = 3.0 / (8.0 * PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg));
        const float spot = smoothstep(0.0, 15.0, phase_sun) * sun_strength;

        return float4(color + spot, 1);

    } else {
        Texture2D skybox = textures[skybox_index];
        const float2 uvs = equi_uvs(normalize(view_vector_worldspace.xyz));
        return skybox.Sample(bilinear_sampler, uvs);
    }
}
