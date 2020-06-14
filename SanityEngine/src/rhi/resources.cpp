#include "resources.hpp"

namespace rhi {
    Uint32 size_in_bytes(const ImageFormat format) {
        switch(format) {
            case ImageFormat::Rgba32F:
                return 16;

            case ImageFormat::Rgba8:
                [[fallthrough]];
            case ImageFormat::R32F:
                [[fallthrough]];
            case ImageFormat::Depth32:
                [[fallthrough]];
            case ImageFormat::Depth24Stencil8:
                [[fallthrough]];
            default:
                return 4;
        }
    }
} // namespace rhi
