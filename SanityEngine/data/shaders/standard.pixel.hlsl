#include "inc/StandardMaterial.hlsli"
#include "inc/lighting.hlsl"
#include "inc/normalmapping.hlsl"

struct VertexOutput {
    float4 location_ndc : SV_POSITION;
    float3 location_worldspace : WORLDPOS;
    float3 normal_worldspace : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

float4 main(VertexOutput input) : SV_TARGET {
    MaterialData material = material_buffer[constants.material_index];

	StandardVertex vertex;
    vertex.position = input.location_worldspace.xyz;
    vertex.normal = normalize(input.normal_worldspace);
    vertex.color = input.color;
    vertex.texcoord = input.texcoord;
	
    const SurfaceInfo surface = get_surface_info(vertex, material);
    
    const Camera camera = cameras[constants.camera_index];
    const Texture2D noise = textures[0];

    float3 total_reflected_light = get_total_reflected_light(camera, surface, noise);

    return float4(total_reflected_light, 1);
}
