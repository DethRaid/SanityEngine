struct DearImGuiVertex {
    float2 position : Position;
    float2 texcoord : Texcoord;
    float4 color : Color;
};

struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
};

struct MaterialData {};

#include "inc/standard_root_signature.hlsl"

VertexShaderOutput main(const DearImGuiVertex input) {
	VertexShaderOutput output;

	const FrameConstants per_frame_data = get_frame_constants();
	const uint2 render_size = per_frame_data.render_size;

    output.position = float4((input.position / render_size) * 2.0 - 1.0, 0, 1);
    output.position.y *= -1.0;
    output.texcoord = input.texcoord;
    output.color = input.color;

    return output;
}
