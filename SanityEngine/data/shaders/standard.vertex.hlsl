#include "mesh_data.hpp"

struct VertexShaderOutput {
    float4 location_ndc : SV_POSITION;
    float3 location_worldspace : WORLDPOS;
    float3 normal_worldspace : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

#include "inc/standard_root_signature.hlsl"

VertexShaderOutput main(StandardVertex input) {
    VertexShaderOutput output;

	const float4x4 model_matrix = get_current_model_matrix();
    output.location_worldspace = mul(model_matrix, float4(input.location, 1.0f)).xyz;
	
    const Camera camera = get_current_camera();
    output.location_ndc = mul(camera.projection, mul(camera.view, float4(output.location_worldspace, 1)));
    
    const float4 normal_worldspace = mul(float4(input.normal, 0), model_matrix);
	
    output.normal_worldspace = normal_worldspace.xyz;
    output.color = input.color;
    output.texcoord = input.texcoord;
    
    return output;
}
