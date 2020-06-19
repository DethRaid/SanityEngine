struct StandardVertex {
    float3 position : Position;
    float3 normal : Normal;
    float4 color : Color;
    float2 texcoord : Texcoord;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float3 position_worldspace : WORLDPOS;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {
    float4x4 model_matrix;
};

#include "inc/standard_root_signature.hlsl"

VertexOutput main(StandardVertex input) {
    VertexOutput output;

    Camera camera = cameras[constants.camera_index];
    MaterialData material = material_buffer[constants.material_index];

    output.position_worldspace = mul(material.model_matrix, float4(input.position, 1)).xyz;
    output.position = mul(camera.projection, mul(camera.view, float4(output.position_worldspace, 1)));
    output.normal = input.normal;
    output.color = input.color;
    output.texcoord = input.texcoord;

    return output;
}
