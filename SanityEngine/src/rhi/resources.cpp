#include "resources.hpp"

#include <D3D12MemAlloc.h>

namespace rhi {
    Buffer::~Buffer() {
        if(allocation) {
            allocation->Release();
        }
    }

    Image::~Image() {
        if(allocation) {
            allocation->Release();
        }
    }

    uint32_t size_in_bytes(const ImageFormat format) {
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
