#pragma once

#include "inc/normalmapping.hlsl"

struct StandardVertex {
    float3 location : Position;
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
    surface.location = vertex.location;

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
        Texture2D normal_texture = textures[material.normal_idx];
        surface.normal = normal_texture.Sample(bilinear_sampler, vertex.texcoord).xyz * 2.0 - 1.0;
        surface.normal.y *= -1; // Convert from right-handed normalmaps to left-handed normals

        FrameConstants data = srv_buffers[0].Load<FrameConstants>(0);
        ByteAddressBuffer cameras = srv_buffers[data.camera_buffer_index];
        const float3 view_vector = vertex.location - cameras.Load<Camera>(constants.camera_index * sizeof(Camera)).view[2].xyz;
    	
        const float3x3 normal_matrix = cotangent_frame(vertex.normal, view_vector, vertex.texcoord);
        surface.normal = normalize(mul(surface.normal, normal_matrix));
            	
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
        surface.roughness = material.metallic_roughness_value.g * material.metallic_roughness_value.g;
    }

    if(material.emission_idx != 0) {
        Texture2D emission_texture = textures[material.emission_idx];
        surface.emission = emission_texture.Sample(bilinear_sampler, vertex.texcoord).rgb;

    } else {
        surface.emission = material.emission_value.rgb;
    }

    return surface;
}
