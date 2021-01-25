#pragma once

struct StandardVertex {
    float3 position : Position;
    float3 normal : Normal;
    float4 color : Color;
    float2 texcoord : Texcoord;
};

struct MaterialData {
    float4 base_color_value;
    float4 metallic_roughness_value;
    float4 emission_value;

    uint base_color_idx;
    uint normal_idx;
    uint metallic_roughness_idx;
    uint emission_idx;
};

struct SurfaceInfo {
    float3 location;

    float4 base_color;

    float3 normal;

    float metalness;

    float roughness;

    float3 emission;
};

#include "standard_root_signature.hlsl"

SurfaceInfo get_surface_info(const in StandardVertex vertex, const in MaterialData material) {
    SurfaceInfo surface;
    surface.location = vertex.position;

    if(material.base_color_idx != 0) {
        Texture2D albedo_texture = textures[material.base_color_idx];
        surface.base_color = albedo_texture.Sample(bilinear_sampler, vertex.texcoord);
        surface.base_color.rgb = pow(surface.base_color.rgb, 1.0 / 2.2);

    } else {
        surface.base_color = material.base_color_value;
    }

	surface.base_color *= vertex.color;

    // TODO: Separate shader (or a shader variant) for alpha-tested objects
    // if(base_color.a == 0) {
    //     // Early-out to avoid the expensive raytrace on completely transparent surfaces
    //     discard;
    // }

    if(material.normal_idx != 0) {
        surface.normal = vertex.normal;
    	
        // TODO: generate normal matrix in fragment shader like a cool kid
        // Texture2D normal_texture = textures[material.normal_idx];
        // surface.normal = normal_texture.Sample(bilinear_sampler, input.texcoord).xyz * 2.0 - 1.0;
        //
        // const float3x3 normal_matrix = cotangent_frame(input.normal_worldspace, input.position_worldspace, input.texcoord);
        // surface.normal = mul(normal_matrix, surface.normal);

    } else {
        surface.normal = vertex.normal;
    }

    if(material.metallic_roughness_idx != 0) {
        Texture2D metallic_roughness_texture = textures[material.metallic_roughness_idx];
        const float4 metallic_roughness = metallic_roughness_texture.Sample(bilinear_sampler, vertex.texcoord);
        surface.metalness = metallic_roughness.b;
        surface.roughness = metallic_roughness.g;

    } else {
        surface.metalness = material.metallic_roughness_value.b;
        surface.roughness = material.metallic_roughness_value.g;
    }

    if(material.emission_idx != 0) {
        Texture2D emission_texture = textures[material.emission_idx];
        surface.emission = emission_texture.Sample(bilinear_sampler, vertex.texcoord).rgb;

    } else {
        surface.emission = material.emission_value.rgb;
    }

    return surface;
}
