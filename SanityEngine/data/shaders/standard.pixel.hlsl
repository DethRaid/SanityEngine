struct VertexOutput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

struct MaterialData {};

#include "inc/standard_root_signature.hlsl"

float4 main(VertexOutput input) : SV_TARGET { 
    Light sun = lights[0];  // The sun is ALWAYS at index 0

    float ndotl = saturate(dot(input.normal, -sun.direction));

    return float4(input.color.rgb * ndotl, input.color.a); 
}