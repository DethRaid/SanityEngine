struct BveVertex {
    float3 position;
    float3 normal;
    float4 color;
    float2 texcoord;
    int double_sided;
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
