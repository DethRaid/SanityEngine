#include "abort.hpp"

#include <Windows.h>
#include <rx/core/string.h>
#include <spdlog/spdlog.h>
#include <stdlib.h>

void critical_error(const Rx::String& message) {
    spdlog::error(message.data());

#ifndef NDEBUG
    DebugBreak();
#else
    system("PAUSE");
    abort();
#endif
}
