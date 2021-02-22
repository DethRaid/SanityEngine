#pragma once

#include "interop.hpp"

#if __cplusplus
namespace sanity::engine::renderer {
#endif
    struct AccumulationMaterial {
        uint accumulation_texture;
        uint scene_output_texture;
        uint scene_depth_texture;
    };
#if __cplusplus
}
#endif