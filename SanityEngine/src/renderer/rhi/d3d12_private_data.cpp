#include "d3d12_private_data.hpp"

namespace renderer::guids {
    void init() {
        CoCreateGuid(&gpu_frame_idx);
        CoCreateGuid(&descriptor_table_handles_guid);
    }
} // namespace renderer::guids
