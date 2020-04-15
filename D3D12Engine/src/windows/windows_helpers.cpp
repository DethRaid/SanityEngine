#include "windows_helpers.hpp"

#include <apiquery2.h>
#include <errhandlingapi.h>
#include <winbase.h>

std::string get_last_windows_error() {
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

    std::string message{message_buffer, size};

    // Free the buffer.
    LocalFree(message_buffer);

    return message;
}
