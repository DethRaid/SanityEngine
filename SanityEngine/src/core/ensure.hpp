#pragma once

#include <Windows.h>
#include <spdlog/spdlog.h>

#ifndef NDEBUG
#define ENSURE(cond, msg, ...)                                                                                                             \
    if(!(cond)) {                                                                                                                          \
        spdlog::error(msg, __VA_ARGS__);                                                                                                   \
        DebugBreak();                                                                                                                      \
    }

#else
#define ENSURE(cond, msg, ...)
#endif
