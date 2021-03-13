#include "fluid_sim.hpp"
#include "inc/standard_root_signature.hlsl"

[numthreads(FLUID_SIM_NUM_THREADS, FLUID_SIM_NUM_THREADS, FLUID_SIM_NUM_THREADS)]
void main(const int3 id : SV_DispatchThreadID) {
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

	const Texture3D pressure_in = textures3d[fluid_volume.pressure_textures[0]];

    const float L = pressure_in[voxel_idx_left].x;
    const float R = pressure_in[voxel_idx_right].x;

    const float B = pressure_in[voxel_idx_bottom].x;
    const float T = pressure_in[voxel_idx_top].x;

    const float D = pressure_in[voxel_idx_back].x;
    const float U = pressure_in[voxel_idx_front].x;
	
	const Texture3D velocity_in = textures3d[fluid_volume.velocity_textures[0]];
	const float3 velocity = velocity_in[id].xyz;
	
	const RWTexture3D<float4> velocity_out = uav_textures3d_rgba[fluid_volume.velocity_textures[1]];
	velocity_out[id].xyz = velocity - float3( R - L, T - B, U - D ) * 0.5;;
}
