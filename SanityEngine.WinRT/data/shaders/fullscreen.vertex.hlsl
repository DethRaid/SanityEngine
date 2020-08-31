struct FullscreenVertexOutput {
    float4 position : SV_POSITION;
    float3 position_clipspace : CLIPPOS;
    float2 texcoord : TEXCOORD;
};

FullscreenVertexOutput main(uint vertex_id : SV_VERTEXID) {
    FullscreenVertexOutput output;
    if(vertex_id % 3 == 0) {
        output.position = float4(-1, -1, 0.99, 1);
        output.texcoord = float2(0, 1);

    } else if(vertex_id % 3 == 1) {
        output.position = float4(-1, 3, 0.99, 1);
        output.texcoord = float2(0, -1);

    } else {
        output.position = float4(3, -1, 0.99, 1);
        output.texcoord = float2(2, 1);
    }

    output.position_clipspace = output.position.xyz;

    return output;
}
