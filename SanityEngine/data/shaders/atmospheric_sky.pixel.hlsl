struct FullscreenVertexOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {};

#include "inc/standard_root_signature.hlsl"

float4 main(FullscreenVertexOutput input) : SV_TARGET {
    return float4(0, 0, 1, 1);
}
