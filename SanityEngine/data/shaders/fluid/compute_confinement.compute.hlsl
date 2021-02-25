#include "inc/standard_root_signature.hlsl"
#include "fluid_sim.hpp"

[numthreads(FLUID_SIM_NUM_THREADS, FLUID_SIM_NUM_THREADS, FLUID_SIM_NUM_THREADS)]
void main(const uint3 id : SV_DispatchThreadID)
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

	const Texture3D vorticity_in = textures3d[fluid_volume.temp_data_buffer];

	const float omega_left = length(vorticity_in[ voxel_idx_left ]);
    const float omega_right = length(vorticity_in[ voxel_idx_right ]);
    
	const float omega_bottom = length(vorticity_in[ voxel_idx_bottom ]);
    const float omega_top = length(vorticity_in[ voxel_idx_top ]);
    
    const float omega_back = length(vorticity_in[ voxel_idx_back ]);
    const float omega_front = length(vorticity_in[ voxel_idx_front ]);    
    
    const float3 omega = vorticity_in[id].xyz;
    
    float3 eta = 0.5 * float3( omega_right - omega_left, omega_top - omega_bottom, omega_front - omega_back );

    eta = normalize( eta + float3(0.001,0.001,0.001) );
    
    float3 force = frame_constants.delta_time * fluid_volume.vorticity_strength * float3( eta.y * omega.z - eta.z * omega.y, eta.z * omega.x - eta.x * omega.z, eta.x * omega.y - eta.y * omega.x );

    const Texture3D velocity_in = textures3d[fluid_volume.velocity_textures[0]];
	const RWTexture3D<float4> velocity_out = uav_textures3d_rgba[fluid_volume.velocity_textures[1]];
	
    velocity_out[id].xyz = velocity_in[id].xyz + force;
}