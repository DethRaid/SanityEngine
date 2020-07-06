#include "d3d12_private_data.hpp"

namespace renderer::guids {
    void init() {
        CoCreateGuid(&gpu_frame_idx);
    }
} // namespace renderer::guids
