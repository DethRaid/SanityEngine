struct DearImGuiVertex {
    float2 position : POSITION;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
};

VertexOutput main( DearImGuiVertex input) {
	VertexOutput output;

    output.position = float4(input.position, 0, 1);
    output.texcoord = input.texcoord;
    output.color = input.color;

    return output;
}
