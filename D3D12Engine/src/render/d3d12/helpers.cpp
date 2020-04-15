#include "helpers.hpp"

#include <d3d12.h>

#include <rx/core/string.h>

void set_object_name(ID3D12Object& object, const std::string& name) {
    const auto wide_name = name.to_utf16();

    object.SetName(reinterpret_cast<LPCWSTR>(wide_name.data()));
}

 DXGI_FORMAT to_dxgi_format(const render::ImageFormat format) {
    switch(format) {
        case render::ImageFormat::Rgba32F:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;

        case render::ImageFormat::Depth32:
            return DXGI_FORMAT_D32_FLOAT;

        case render::ImageFormat::Depth24Stencil8:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;

        case render::ImageFormat::Rgba8:
            [[fallthrough]];
        default:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
    }
}
