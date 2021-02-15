#pragma once

#include <filesystem>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "adapters/rex/stdout_stream.hpp"
#include "core/Prelude.hpp"

// This include is necessary, even though VS doesn't realize it
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
        const char* operator()(const std::string& data) const;
    };

    template <>
    struct FormatNormalize<std::filesystem::path> {
        static inline const size_t MAX_PATH_SIZE = 260;

        char scratch[MAX_PATH_SIZE];

        const char* operator()(const std::filesystem::path& data);
    };

    template <>
    struct FormatNormalize<glm::vec3> {
        char scratch[FormatSize<Float32>::size * 3 + sizeof "(, , )" - 1];
        const char* operator()(const glm::vec3& data);
    };

    template <>
    struct FormatNormalize<glm::quat> {
        char scratch[FormatSize<Float32>::size * 3 + sizeof "(, , )" - 1];
        const char* operator()(const glm::quat& data);
    };
} // namespace Rx
