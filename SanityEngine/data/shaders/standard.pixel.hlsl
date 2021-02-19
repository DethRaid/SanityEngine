#include "inc/StandardMaterial.hlsli"
#include "inc/lighting.hlsl"

struct VertexOutput {
    float4 location_ndc : SV_POSITION;
    float3 location_worldspace : WORLDPOS;
    float3 normal_worldspace : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

struct PixelOutput {
    float4 color : SV_TARGET0;
    uint object_id : SV_TARGET1;
};

PixelOutput main(const VertexOutput input) {
	GET_CURRENT_DATA(MaterialData, material);
	
    StandardVertex vertex;
    vertex.location = input.location_worldspace.xyz;
    vertex.normal = normalize(input.normal_worldspace);
    vertex.color = input.color;
    vertex.texcoord = input.texcoord;

    const SurfaceInfo surface = get_surface_info(vertex, material);

    const Camera camera = get_current_camera();

    float3 total_reflected_light = get_total_reflected_light(camera, surface);

	PixelOutput output;
    output.color = float4(total_reflected_light, 1);
    output.object_id = constants.object_id;
        
	return output;
}
