#include "abort.hpp"

#include <Windows.h>
#include <spdlog/spdlog.h>

void critical_error(const std::string& message) {
    spdlog::error(message);

#ifndef NDEBUG
    DebugBreak();
#else
    abort();
#endif
}
