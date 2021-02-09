#include "resources.hpp"

namespace sanity::engine::renderer {
    Uint32 size_in_bytes(const TextureFormat format) {
        switch(format) {
            case TextureFormat::Rgba32F:
                return 16;

            case TextureFormat::Rgba8:
                [[fallthrough]];
            case TextureFormat::R32F:
                [[fallthrough]];
            case TextureFormat::Depth32:
                [[fallthrough]];
            case TextureFormat::Depth24Stencil8:
                [[fallthrough]];
            default:
                return 4;
        }
    }
} // namespace rhi
