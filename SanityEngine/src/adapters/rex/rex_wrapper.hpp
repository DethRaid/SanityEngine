#pragma once

// This include is necessary, even though VS doesn't realize it
#include <filesystem>

#include "adapters/rex/stdout_stream.hpp"
#include "core/Prelude.hpp"
#include "rx/core/memory/system_allocator.h"

namespace rex {
    class Wrapper {
    public:
        Wrapper();

        ~Wrapper();

    private:
        StdoutStream stdout_stream;
    };
} // namespace rex

// My linter wants this to be a constexpr variable. That doesn't work, however. I don't know exactly why but VS gives
// me a lot of red squigglies
#define RX_SYSTEM_ALLOCATOR Rx::Memory::SystemAllocator::instance() // NOLINT(cppcoreguidelines-macro-usage)

constexpr bool RX_ITERATION_CONTINUE = true;
constexpr bool RX_ITERATION_STOP = false;

namespace Rx {
    template <>
    struct FormatNormalize<std::string> {
        const char* operator()(const std::string& data) const { return data.c_str(); }
    };

    template <>
    struct FormatNormalize<std::filesystem::path> {
        static inline const size_t max_path_size = 260;
    	
    	char scratch[max_path_size]; // haha WIndows path
        const char* operator()(const std::filesystem::path& data) {
            const auto path_string = data.string();
            const auto result = memcpy_s(scratch, max_path_size * sizeof(char), path_string.c_str(), path_string.size() * sizeof(char));
            if(result != 0) {
                fprintf(stderr, "Could not format std::filesystem::path %s: error code %d", path_string.c_str(), result);
                memset(scratch, 0, 260 * sizeof(char));
            }
        	
            return scratch;
        }
    };
} // namespace Rx
