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

VertexOutput main(BveVertex input) {
    VertexOutput output;

    output.position  = float4(input.position, 1);
    output.normal = input.normal;
    output.color = input.color;
    output.texcoord = input.texcoord;

    return output;
}
