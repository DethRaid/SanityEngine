#include "abort.hpp"

#include <string>

#include <debugapi.h>
#include <spdlog/spdlog.h>

void critical_error(const std::string& message) {
    spdlog::error(message);

#ifndef NDEBUG
    DebugBreak();
#else
    abort();
#endif
}
