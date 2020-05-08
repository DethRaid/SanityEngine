#include "resources.hpp"

namespace rhi {
    uint32_t size_in_bytes(const ImageFormat format) {
        switch(format) {
            case ImageFormat::Rgba32F:
                return 32;

            case ImageFormat::Rgba8:
                [[fallthrough]];
            case ImageFormat::Depth32:
                [[fallthrough]];
            case ImageFormat::Depth24Stencil8:
                [[fallthrough]];
            default:
                return 8;
        }
    }
} // namespace render
