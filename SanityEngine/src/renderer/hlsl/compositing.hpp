#ifndef COMPOSITING_HPP
#define COMPOSITING_HPP

#include "interop.hpp"

#ifndef __cplusplus
namespace sanity::engine::renderer {
#endif

    /**
     * @brief All the textures that need to be composited together, and any parameters for compositing them
     */
    struct CompositingTextures
    {
        uint direct_lighting_idx;
        uint fluid_color_idx;

        uint output_idx;
    };

#ifndef __cplusplus
}
#endif

#endif
