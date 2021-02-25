#include "inc/standard_root_signature.hlsl"
#include "fluid_sim.hpp"

[numthreads(FLUID_SIM_NUM_THREADS, FLUID_SIM_NUM_THREADS, FLUID_SIM_NUM_THREADS)]
void main(int3 id : SV_DispatchThreadID) {
    const FrameConstants frame_constants = get_frame_constants();
    GET_CURRENT_DATA(GpuFluidVolumeState, fluid_volume);

	Texture3D density_in = textures3d[fluid_volume.density_textures[0]];
	Texture3D temperature_in = textures3d[fluid_volume.temperature_textures[0]];
	Texture3D velocity_in = textures3d[fluid_volume.velocity_textures[0]];

	RWTexture3D<float4> velocity_out = uav_textures3d_rgba[fluid_volume.velocity_textures[1]];
    
	const float3 my_uv = float3(id) / float3(fluid_volume.voxel_size.xyz);
    
	const float temperature = temperature_in[id].x;
	if(temperature > frame_constants.ambient_temperature)
	{
		const float density = density_in[id].x;
		const float3 velocity = velocity_in[id].xyz;
		const float3 adjusted_velocity = velocity + (frame_constants.delta_time * (temperature - frame_constants.ambient_temperature) * fluid_volume.buoyancy - density * fluid_volume.weight) * float3(0, 1, 0);
		velocity_out[id].xyz = adjusted_velocity;		
	}
}
