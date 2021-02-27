#include "fluid_sim.hpp"
#include "inc/standard_root_signature.hlsl"

[numthreads(FLUID_SIM_NUM_THREADS, FLUID_SIM_NUM_THREADS, FLUID_SIM_NUM_THREADS)]
void main( const uint3 id : SV_DispatchThreadID )
{
    const FrameConstants frame_constants = get_frame_constants();
    GET_CURRENT_DATA(GpuFluidVolumeState, fluid_volume);

    const int3 voxel_idx = id;

    int3 voxel_idx_left = voxel_idx + int3(-1, 0, 0);
    voxel_idx_left.x = max(0.f, voxel_idx_left.x);

    int3 voxel_idx_right = voxel_idx + int3(1, 0, 0);
    voxel_idx_right.x = min(fluid_volume.size.x - 1, voxel_idx_right.x);

    int3 voxel_idx_bottom = voxel_idx + int3(0, -1, 0);
    voxel_idx_bottom.y = max(0, voxel_idx_bottom.y);

    int3 voxel_idx_top = voxel_idx + int3(0, 1, 0);
    voxel_idx_top.y = min(fluid_volume.size.y - 1, voxel_idx_top.y);

    int3 voxel_idx_back = voxel_idx + int3(0, 0, -1);
    voxel_idx_back.z = max(0, voxel_idx_back.z);

    int3 voxel_idx_front = voxel_idx + int3(0, 0, 1);
    voxel_idx_front.z = min(fluid_volume.size.z - 1, voxel_idx_front.z);

    const Texture3D velocity_in = textures3d[fluid_volume.velocity_textures[0]];

    const float3 L = velocity_in[voxel_idx_left].xyz;
    const float3 R = velocity_in[voxel_idx_right].xyz;

    const float3 B = velocity_in[voxel_idx_bottom].xyz;
    const float3 T = velocity_in[voxel_idx_top].xyz;

    const float3 D = velocity_in[voxel_idx_back].xyz;
    const float3 U = velocity_in[voxel_idx_front].xyz;

    const float divergence = 0.5 * ((R.x - L.x) + (T.y - B.y) + (U.z - D.z));

    const RWTexture3D<float>  temp_buffer = uav_textures3d_r[fluid_volume.temp_data_buffer];
    temp_buffer[id] = divergence;
}
