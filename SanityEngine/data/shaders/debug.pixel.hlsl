struct VertexOutput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

float4 main(VertexOutput input) : SV_TARGET { return input.color; }