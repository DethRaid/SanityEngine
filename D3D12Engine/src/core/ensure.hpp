#pragma once

#include <spdlog/spdlog.h>
#include <assert.h>

#ifndef NDEBUG
#define ENSURE(cond, msg, ...)                  \
    if(!(cond)) {                               \
         spdlog::error(msg, __VA_ARGS__);       \
         assert(false);                         \
    }
   
#else
#define ENSURE(cond, msg, ...)
#endif
