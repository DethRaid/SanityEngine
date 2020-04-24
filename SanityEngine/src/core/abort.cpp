#include "abort.hpp"

#include <Windows.h>
#include <spdlog/spdlog.h>
#include <stdlib.h>

void critical_error(const std::string& message) {
    spdlog::error(message);

#ifndef NDEBUG
    DebugBreak();
#else
    system("PAUSE");
    abort();
#endif
}
