#include "interop.hpp"

#include "inc/standard_root_signature.hlsl"
#include "fluid_sim.hpp"

[numthreads(FLUID_SIM_NUM_THREADS, FLUID_SIM_NUM_THREADS, FLUID_SIM_NUM_THREADS)]
void main( uint3 id : SV_DispatchThreadID ) {
	const FrameConstants frame_constants = get_frame_constants();
	GET_CURRENT_DATA(GpuFluidVolumeState, fluid_volume);
		
	const float3 emitter_to_voxel = id / (fluid_volume.size.xyz - 1.f) - fluid_volume.emitter_location.xyz;
	const float mag = dot(emitter_to_voxel, emitter_to_voxel);
	const float rad2 = fluid_volume.emitter_radius * fluid_volume.emitter_radius;
	const float amount = exp2(-mag/rad2) * fluid_volume.emitter_strength * frame_constants.delta_time;
	
	const Texture3D temperature_in = textures3d[fluid_volume.temperature_textures[0]];
	const RWTexture3D<float> temperature_out = uav_textures3d_r[fluid_volume.temperature_textures[1]];
	
	temperature_out[id] = temperature_in[id].r + amount;
	
	const Texture3D reaction_in = textures3d[fluid_volume.reaction_textures[0]];
	const RWTexture3D<float> reaction_out = uav_textures3d_r[fluid_volume.reaction_textures[1]];
	
	reaction_out[id] = reaction_in[id].r + amount;
}
