#include "resources.hpp"

namespace sanity::engine::renderer {
    Uint32 size_in_bytes(const TextureFormat format) {
        switch(format) {
            case TextureFormat::Rgba8:
                [[fallthrough]];
            case TextureFormat::Rg16F:
                [[fallthrough]];
            case TextureFormat::R32F:
                [[fallthrough]];
            case TextureFormat::R32UInt:
                [[fallthrough]];
            case TextureFormat::Depth32:
                [[fallthrough]];
            case TextureFormat::Depth24Stencil8:
                return 4;

        	case TextureFormat::Rg32F:
                return 8;
        	
            case TextureFormat::Rgba32F:
                return 16;

            default:
                return 4;
        }
    }
} // namespace rhi
