struct DearImGuiVertex {
    float2 position : Position;
    float2 texcoord : Texcoord;
    float4 color : Color;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
};

VertexOutput main( DearImGuiVertex input) {
	VertexOutput output;

    output.position = float4((input.position / float2(1000, 480)) * 2.0 - 1.0, 0, 1);
    output.position.y *= -1.0;
    output.texcoord = input.texcoord;
    output.color = input.color;

    return output;
}
