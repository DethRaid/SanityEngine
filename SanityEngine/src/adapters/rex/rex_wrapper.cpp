#include "rex_wrapper.hpp"

#if TRACY_ENABLE
#include "Tracy.hpp"
#include "client/tracy_concurrentqueue.h"
#include "rx/core/profiler.h"

using namespace tracy;
#endif

#include "rx/core/abort.h"
#include "rx/core/global.h"
#include "rx/core/log.h"

namespace rex {
#if TRACY_ENABLE
    void SetThreadName(void* /*_context*/, const char* _name) { tracy::SetThreadName(_name); }

    void BeginSample(void* /*_context*/, const Rx::Profiler::Sample* _sample) {
        const auto& source_info = _sample->source_location();

        // Enframe tracy::SourceLocationData in the Sample.
        auto* enframe = _sample->enframe<SourceLocationData>();
        enframe->name = _sample->tag();
        enframe->function = source_info.function();
        enframe->file = source_info.file();
        enframe->line = source_info.line();

        TracyLfqPrepare(tracy::QueueType::ZoneBegin);
        MemWrite(&item->zoneBegin.time, Profiler::GetTime());
        MemWrite(&item->zoneBegin.srcloc, (uint64_t) enframe);
        TracyLfqCommit;
    }

    void EndSample(void* /*_context*/, const Rx::Profiler::Sample* /*_sample*/) {
        TracyLfqPrepare(tracy::QueueType::ZoneEnd);
        MemWrite(&item->zoneEnd.time, Profiler::GetTime());
        TracyLfqCommit;
    }
#endif

    Wrapper::Wrapper() {
        const auto globals_linked = Rx::Globals::link();
        if(!globals_linked) {
            Rx::abort("Could not link the Rex globals");
        }

        if(!Rx::Log::subscribe(&stdout_stream)) {
            Rx::abort("Could not subscribe stdout to Rex's logger");
        }

#if TRACY_ENABLE
        Rx::Profiler::instance().bind_cpu({nullptr, SetThreadName, BeginSample, EndSample});
#endif
    }

    Wrapper::~Wrapper() {
#if TRACY_ENABLE
        Rx::Profiler::instance().unbind_cpu();
#endif

        Rx::Globals::fini();
    }
} // namespace rex

namespace Rx {
    const char* FormatNormalize<std::string>::operator()(const std::string& data) const { return data.c_str(); }

    const char* FormatNormalize<std::filesystem::path>::operator()(const std::filesystem::path& data) {
        const auto path_string = data.string();
        const auto result = memcpy_s(scratch, MAX_PATH_SIZE * sizeof(char), path_string.c_str(), path_string.size() * sizeof(char));
        if(result != 0) {
            fprintf(stderr, "Could not format std::filesystem::path %s: error code %d", path_string.c_str(), result);
            memset(scratch, 0, 260 * sizeof(char));
        }

        return scratch;
    }

    const char* FormatNormalize<glm::vec3>::operator()(const glm::vec3& data) {
        sprintf_s(scratch, "(%f, %f, %f)", static_cast<double>(data.x), static_cast<double>(data.y), static_cast<double>(data.z));

        return scratch;
    }

    const char* FormatNormalize<glm::vec4>::operator()(const glm::vec4& data)
    {
          sprintf_s(scratch, "(%f, %f, %f, %f)", static_cast<double>(data.x), static_cast<double>(data.y), static_cast<double>(data.z), static_cast<double>(data.w));

        return scratch;
    }

    const char* FormatNormalize<glm::quat>::operator()(const glm::quat& data) {
        const auto euler = glm::eulerAngles(data);
        sprintf_s(scratch, "(%f, %f, %f)", static_cast<double>(euler.x), static_cast<double>(euler.y), static_cast<double>(euler.z));

        return scratch;
    }

    const char* FormatNormalize<glm::mat4>::operator()(const glm::mat4& data)
    {
        auto formatter_0 = FormatNormalize<glm::vec4>{};
        auto formatter_1 = FormatNormalize<glm::vec4>{};
        auto formatter_2 = FormatNormalize<glm::vec4>{};
        auto formatter_3 = FormatNormalize<glm::vec4>{};

          sprintf_s(scratch, "\n[%s\n %s\n %s\n %s]", formatter_0(data[0]), formatter_1(data[1]), formatter_2(data[2]), formatter_3(data[3]));

        return scratch;
    }
} // namespace Rx
