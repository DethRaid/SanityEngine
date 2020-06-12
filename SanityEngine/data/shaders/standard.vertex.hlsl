struct StandardVertex {
    float3 position : Position;
    float3 normal : Normal;
    float4 color : Color;
    uint material_index : MaterialIndex;
    float2 texcoord : Texcoord;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float3 position_worldspace : WORLDPOS;
    float3 normal : NORMAL;
    float4 color : COLOR;
    uint material_index : MATERIALINDEX;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {};

#include "inc/standard_root_signature.hlsl"

VertexOutput main(StandardVertex input) {
    VertexOutput output;

    Camera camera = cameras[constants.camera_index];

    output.position = mul(camera.projection, mul(camera.view, float4(input.position, 1)));
    output.position_worldspace = input.position;
    output.normal = input.normal;
    output.color = input.color;
    output.material_index = input.material_index;
    output.texcoord = input.texcoord;

    return output;
}
