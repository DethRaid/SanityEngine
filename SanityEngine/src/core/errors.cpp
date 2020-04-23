#include "errors.hpp"

#include <comdef.h>

std::string to_string(const HRESULT hr) {
    const _com_error err(hr);
    return err.ErrorMessage();
}
