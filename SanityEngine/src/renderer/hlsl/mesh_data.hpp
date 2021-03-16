#ifndef MESH_DATA
#define MESH_DATA

#include "interop.hpp"

#if __cplusplus
namespace sanity::engine::renderer {
#endif

    struct StandardVertex {
        float3 location LOCATION_SEMANTIC;
        float3 normal NORMAL_SEMANTIC;
        byte4 color COLOR_SEMANTIC;
        float2 texcoord TEXCOORD_SEMANTIC;
    };

#if __cplusplus
    static_assert(sizeof(StandardVertex) == 8 * sizeof(float) + sizeof(Uint32));
    static_assert(alignof(StandardVertex) == 4);
}
#endif

#if !__cplusplus

#include "inc/standard_root_signature.hlsl"
#include "shared_structs.hpp"

#define BYTES_PER_VERTEX sizeof(StandardVertex)

uint3 get_indices(uint triangle_index) {
    const uint base_index = (triangle_index * 3);
    const int address = (base_index * 4);

    FrameConstants data = get_frame_constants();
    ByteAddressBuffer indices = srv_buffers[data.index_buffer_index];
    return indices.Load3(address);
}

StandardVertex get_vertex(int address) {
    FrameConstants data = get_frame_constants();
    ByteAddressBuffer vertices = srv_buffers[data.vertex_data_buffer_index];

    StandardVertex v = vertices.Load<StandardVertex>(address);
    v.location = asfloat(vertices.Load3(address));
    address += (3 * 4);
    v.normal = asfloat(vertices.Load3(address));
    address += (3 * 4);
    v.color = asfloat(vertices.Load4(address));
    address += (4); // Vertex colors are only only one byte per component
    v.texcoord = asfloat(vertices.Load2(address));

    return v;
}

StandardVertex get_vertex_attributes(uint triangle_index, float2 barycentrics) {
    uint3 indices = get_indices(triangle_index);

    StandardVertex v0 = get_vertex((indices[0] * BYTES_PER_VERTEX) * 4);
    StandardVertex v1 = get_vertex((indices[1] * BYTES_PER_VERTEX) * 4);
    StandardVertex v2 = get_vertex((indices[2] * BYTES_PER_VERTEX) * 4);

    StandardVertex v;
    v.location = v0.location + barycentrics.x * (v1.location - v0.location) + barycentrics.y * (v2.location - v0.location);
    v.normal = v0.normal + barycentrics.x * (v1.normal - v0.normal) + barycentrics.y * (v2.normal - v0.normal);
    v.color = v0.color + barycentrics.x * (v1.color - v0.color) + barycentrics.y * (v2.color - v0.color);
    v.texcoord = v0.texcoord + barycentrics.x * (v1.texcoord - v0.texcoord) + barycentrics.y * (v2.texcoord - v0.texcoord);

    return v;
}
#endif

#endif
