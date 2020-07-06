#pragma once

#include <rx/core/memory/system_allocator.h>

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
        inline static bool initialized{false};
    };
} // namespace rex

#define RX_SYSTEM_ALLOCATOR Rx::Memory::SystemAllocator::instance()

constexpr bool RX_ITERATION_CONTINUE = true;
constexpr bool RX_ITERATION_STOP = false;
