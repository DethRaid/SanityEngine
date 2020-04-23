struct VertexOutput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    uint color : COLOR;
    float2 texcoord : TEXCOORD;
    int double_sided : DATA;
};

float4 main(VertexOutput input) : SV_TARGET {
    return input.position;
}