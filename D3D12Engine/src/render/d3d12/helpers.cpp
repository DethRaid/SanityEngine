#include "helpers.hpp"

#include <d3d12.h>

std::wstring to_wide_string(const std::string& string) {
    const int wide_string_length = MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, nullptr, 0);
    wchar_t* wide_char_string = new wchar_t[wide_string_length];
    MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, wide_char_string, wide_string_length);

    std::wstring wide_string{wide_char_string};

    delete[] wide_char_string;

    return wide_string;
}

void set_object_name(ID3D12Object& object, const std::string& name) {
    const auto wide_name = to_wide_string(name);

    object.SetName(reinterpret_cast<LPCWSTR>(wide_name.c_str()));
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
