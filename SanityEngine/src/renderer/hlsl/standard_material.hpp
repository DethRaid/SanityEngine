#pragma once

#include "interop.hpp"

#if __cplusplus

#include "renderer/handles.hpp"

namespace sanity::engine::renderer {
#endif

	struct StandardMaterial {
        float4 base_color_value;
        float4 metallic_roughness_value;
        float4 emission_value;

        uint base_color_texture_idx;
        uint normal_texture_idx;
        uint metallic_roughness_texture_idx;
        uint emission_texture_idx;
    };

	#if __cplusplus
	
	using StandardMaterialHandle = GpuResourceHandle<StandardMaterial>;
}
#endif
