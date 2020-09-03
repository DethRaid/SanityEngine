#include "errors.hpp"

#include <comdef.h>

#include "renderer/rhi/helpers.hpp"

Rx::String to_string(const HRESULT hr) {
    const _com_error err(hr);
    const auto error_string_utf16 = Rx::WideString{reinterpret_cast<const Uint16*>(err.ErrorMessage())};
    const auto error_string_utf8 = error_string_utf16.to_utf8();
    return Rx::String::format("%s (error code 0x%x)", error_string_utf8, hr);
}
