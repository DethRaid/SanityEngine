#include "inc/standard_root_signature.hlsl"
#include "fluid_sim.hpp"

[numthreads(FLUID_SIM_NUM_THREADS, FLUID_SIM_NUM_THREADS, FLUID_SIM_NUM_THREADS)]
void main(uint3 id : SV_DispatchThreadID) {
	const FrameConstants frame_constants = get_frame_constants();
	GET_CURRENT_DATA(GpuFluidVolumeState, fluid_volume);

	const Texture3D reaction_in = textures3d[fluid_volume.reaction_textures[0]];

	const float reaction = reaction_in[id].x;
	if(reaction > 0.f && reaction < fluid_volume.reaction_extinguishment) {
		const Texture3D density_in = textures3d[fluid_volume.density_textures[0]];
		const RWTexture3D<float> density_out = uav_textures3d_r[fluid_volume.density_textures[2]];

		density_out[id] = density_in[id] + fluid_volume.density_extinguishment_amount * reaction;
	}
}
