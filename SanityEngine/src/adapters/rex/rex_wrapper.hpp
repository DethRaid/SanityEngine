#pragma once

// This include is necessary, even though VS doesn't realize it
#include "rx/core/memory/system_allocator.h"
#include "core/Prelude.hpp"
#include "adapters/rex/stdout_stream.hpp"

namespace rex {
    class Wrapper {
    public:
        Wrapper();

        Wrapper(const Wrapper& other) = delete;
        Wrapper& operator=(const Wrapper& other) = delete;

        Wrapper(Wrapper&& old) noexcept = default;
        Wrapper& operator=(Wrapper&& old) noexcept = default;

        ~Wrapper();

    private:
        StdoutStream stdout_stream;
    };
} // namespace rex

#define RX_SYSTEM_ALLOCATOR Rx::Memory::SystemAllocator::instance()

constexpr bool RX_ITERATION_CONTINUE = true;
constexpr bool RX_ITERATION_STOP = false;
