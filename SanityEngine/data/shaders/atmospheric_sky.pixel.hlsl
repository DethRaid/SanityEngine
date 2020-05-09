struct FullscreenVertexOutput {
    float4 position : SV_POSITION;
    float3 position_viewspace : VIEWPOS;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {
};

#include "inc/standard_root_signature.hlsl"

// Sky options
#define RAYLEIGH_BRIGHTNESS         3.3
#define MIE_BRIGHTNESS              0.1
#define MIE_DISTRIBUTION            0.63
#define STEP_COUNT                  15.0
#define SCATTER_STRENGTH            0.028
#define RAYLEIGH_STRENGTH           0.139
#define MIE_STRENGTH                0.0264
#define RAYLEIGH_COLLECTION_POWER	0.81
#define MIE_COLLECTION_POWER        0.39

#define SUNSPOT_BRIGHTNESS          500
#define MOONSPOT_BRIGHTNESS         25

#define SKY_SATURATION              1.5

#define SURFACE_HEIGHT              0.98

#define CLOUDS_START                512

#define PI 3.14159

/*
 * Begin sky rendering code
 *
 * Taken from http://codeflow.org/entries/2011/apr/13/advanced-webgl-part-2-sky-rendering/
 */

float phase(float alpha, float g) {
    float a = 3.0 * (1.0 - g * g);
    float b = 2.0 * (2.0 + g * g);
    float c = 1.0 + alpha * alpha;
    float d = pow(1.0 + g * g - 2.0 * g * alpha, 1.5);
    return (a / b) * (c / d);
}

float atmospheric_depth(float3 position, float3 dir) {
    float a = dot(dir, dir);
    float b = 2.0 * dot(dir, position);
    float c = dot(position, position) - 1.0;
    float det = b * b - 4.0 * a * c;
    float detSqrt = sqrt(det);
    float q = (-b - detSqrt) / 2.0;
    float t1 = c / q;
    return t1;
}

float horizon_extinction(float3 position, float3 dir, float radius) {
    float u = dot(dir, -position);
    if(u < 0.0) {
        return 1.0;
    }

    float3 near = position + u*dir;

    if(sqrt(dot(near, near)) < radius) {
        return 0.0;

    } else {
        float3 v2 = normalize(near)*radius - position;
        float diff = acos(dot(normalize(v2), dir));
        return smoothstep(0.0, 1.0, pow(diff * 2.0, 3.0));
    }
}

static const float3 Kr = float3(0.18867780436772762, 0.4978442963618773, 0.6616065586417131);    // Color of nitrogen

float3 absorb(float dist, float3 color, float factor) {
    return color - color * pow(Kr, (factor / dist).xxx);
}

float3 get_sky_color(in float3 eye_vector, in float3 light_vector, in float light_intensity, bool show_light) {
    float alpha = max(dot(eye_vector, light_vector), 0.0);

    float rayleigh_factor = phase(alpha, -0.01) * RAYLEIGH_BRIGHTNESS;
    float mie_factor = phase(alpha, MIE_DISTRIBUTION) * MIE_BRIGHTNESS;
    float spot = show_light ? smoothstep(0.0, 15.0, phase(alpha, 0.9995)) * light_intensity : 0;

    float3 eye_position = float3(0.0, SURFACE_HEIGHT, 0.0);
    float eye_depth = atmospheric_depth(eye_position, eye_vector);
    float step_length = eye_depth / STEP_COUNT;

    float eye_extinction = horizon_extinction(eye_position, eye_vector, SURFACE_HEIGHT - 0.15);

    float3 rayleigh_collected = 0;
    float3 mie_collected = 0;

    for(int i = 0; i < STEP_COUNT; i++) {
        float sample_distance = step_length * float(i);
        float3 position = eye_position + eye_vector * sample_distance;
        float extinction = horizon_extinction(position, light_vector, SURFACE_HEIGHT - 0.35);
        float sample_depth = atmospheric_depth(position, light_vector);

        float3 influx = absorb(sample_depth, float3(light_intensity.xxx), SCATTER_STRENGTH) * extinction;

        // rayleigh will make the nice blue band around the bottom of the sky
        rayleigh_collected += absorb(sample_distance, Kr * influx, RAYLEIGH_STRENGTH);
        mie_collected += absorb(sample_distance, influx, MIE_STRENGTH);
    }

    rayleigh_collected = (rayleigh_collected * eye_extinction * pow(eye_depth, RAYLEIGH_COLLECTION_POWER)) / STEP_COUNT;
    mie_collected = (mie_collected * eye_extinction * pow(eye_depth, MIE_COLLECTION_POWER)) / STEP_COUNT;

    float3 color = (spot * mie_collected) + (mie_factor * mie_collected) + (rayleigh_factor * rayleigh_collected);// * float3(1, 0.8, 0.8);

    return color;
}

float4 main(FullscreenVertexOutput input) : SV_TARGET {
    Camera camera = cameras[constants.camera_index];
    float4 view_vector_worldspace = mul(camera.inverse_view, float4(input.position_viewspace, 0));

    Light light = lights[0];    // Light 0 is always the sun

    float3 sky_color = get_sky_color(normalize(view_vector_worldspace.xyz), -normalize(light.direction), length(light.color), true);

    return float4(sky_color, 1);
}
