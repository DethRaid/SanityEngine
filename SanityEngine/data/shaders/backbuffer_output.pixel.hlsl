struct FullscreenVertexOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

/*
 * \brief Scene output render target
 */
Texture2D<float4> scene_output : register(t0);

SamplerState bilinear_sampler : register(s0, space0);

float4 main(FullscreenVertexOutput vertex) : SV_TARGET {
    return scene_output.Sample(bilinear_sampler, vertex.texcoord);
}
