// Allow macros because HLSL
// ReSharper disable CppClangTidyCppcoreguidelinesMacroUsage
#ifndef COMPOSITING_HPP
#define COMPOSITING_HPP

#include "interop.hpp"

#if __cplusplus
namespace sanity::engine::renderer {
#endif

    #define COMPOSITING_NUM_THREADS 8

    /**
     * @brief All the textures that need to be composited together, and any parameters for compositing them
     */
    struct CompositingTextures
    {
        uint direct_lighting_idx;
        uint fluid_color_idx;

        uint output_idx;
    };

#if __cplusplus
}
#endif

#endif
