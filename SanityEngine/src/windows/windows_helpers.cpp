#include "windows_helpers.hpp"

#include <Windows.h>
#include <comdef.h>

#include "core/types.hpp"

Rx::String to_string(const HRESULT hr) {
    const _com_error err{hr};
    const auto error_string_utf16 = Rx::WideString{reinterpret_cast<const Uint16*>(err.ErrorMessage())};
    const auto error_string_utf8 = error_string_utf16.to_utf8();
    return Rx::String::format("%s (error code 0x%x)", err.ErrorMessage(), hr);
}

Rx::String get_last_windows_error() {
    const DWORD error_message_id = GetLastError();
    if(error_message_id == 0) {
        return {}; // No error message has been recorded
    }

    LPSTR message_buffer = nullptr;
    const size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                       nullptr,
                                       error_message_id,
                                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                       reinterpret_cast<LPSTR>(&message_buffer),
                                       0,
                                       nullptr);

    Rx::String message{message_buffer, size};

    // Free the buffer.
    LocalFree(message_buffer);

    return message;
}
