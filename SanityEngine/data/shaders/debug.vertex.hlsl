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

    output.position = mul(float4(input.position, 1), mul(camera.view, camera.projection));
    output.normal = input.normal;
    output.color = float4(1, 0, 1, 1);//input.color;
    output.texcoord = input.texcoord;

    return output;
}
