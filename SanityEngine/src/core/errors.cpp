#include "errors.hpp"

#include <comdef.h>

#include "rhi/helpers.hpp"

Rx::String to_string(const HRESULT hr) {
    const _com_error err(hr);
    return Rx::String::format("%s (error code %x)", Rx::WideString{reinterpret_cast<const Uint16*>(err.ErrorMessage())}.to_utf8(), hr);
}
