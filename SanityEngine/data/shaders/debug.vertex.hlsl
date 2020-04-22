struct BveVertex {
    float3 position : Position;
    float3 normal : Normal;
    float4 color : Color;
    float2 texcoord : Texcoord;
    int double_sided : DoubleSided;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
    int double_sided : DATA;
};

VertexOutput main(BveVertex input) {
    VertexOutput output;

    output.position  = float4(input.position, 1);
    output.normal = input.normal;
    output.color = input.color;
    output.texcoord = input.texcoord;
    output.double_sided = input.double_sided;

    return output;
}
