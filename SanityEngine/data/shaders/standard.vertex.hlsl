struct BveVertex {
    float3 position : Position;
    float3 normal : Normal;
    float4 color : Color;
    float2 texcoord : Texcoord;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {};

#include "inc/standard_root_signature.hlsl"

VertexOutput main(BveVertex input) {
    VertexOutput output;

    Camera camera = cameras[constants.camera_index];

    output.position = mul(camera.projection, mul(camera.view, float4(input.position, 1)));
    output.normal = input.normal;
    output.color = input.color;
    output.texcoord = input.texcoord;

    return output;
}
