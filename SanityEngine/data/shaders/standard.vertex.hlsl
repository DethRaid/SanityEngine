struct StandardVertex {
    float3 location_modelspace : Position;
    float3 normal_modelspace : Normal;
    float4 color : Color;
    float2 texcoord : Texcoord;
};

struct VertexOutput {
    float4 location_ndc : SV_POSITION;
    float3 location_worldspace : WORLDPOS;
    float3 normal_worldspace : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {};

#include "inc/standard_root_signature.hlsl"

VertexOutput main(StandardVertex input) {
    VertexOutput output;

	const float4x4 model_matrix = model_matrices[constants.model_matrix_index];
    output.location_worldspace = mul(model_matrix, float4(input.location_modelspace, 1.0f)).xyz;
	
    const Camera camera = cameras[constants.camera_index];
    output.location_ndc = mul(camera.projection, mul(camera.view, float4(output.location_worldspace, 1)));
    
    const float4 normal_worldspace = mul(float4(input.normal_modelspace, 0), model_matrix);
	
    output.normal_worldspace = normal_worldspace.xyz;
    output.color = input.color;
    output.texcoord = input.texcoord;
    
    return output;
}
