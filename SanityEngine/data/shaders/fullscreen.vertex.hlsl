struct FullscreenVertexOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

FullscreenVertexOutput main(uint vertex_id : SV_VERTEXID) {
    // Use a w = 0 so that things draws with this geometry map to the far clipping plane
    // See page 8 of http://www.terathon.com/gdc07_lengyel.pdf for details

    FullscreenVertexOutput output;
    if(vertex_id % 3 == 0) {
        output.position = float4(-1, -1, 1, 0);
        output.texcoord = float2(0, 1);

    } else if(vertex_id % 3 == 1) {
        output.position = float4(0, 3, 1, 0);
        output.texcoord = float2(0, -2);

    } else {
        output.position = float4(3, 0, 1, 0);
        output.texcoord = float2(2, 0);
    }

    return output;
}
