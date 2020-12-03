#include "RexWrapper.hpp"

#if TRACY_ENABLE
#include "Tracy.hpp"
#include "rx/core/profiler.h"
#endif

#include "Rex/StdoutStream.hpp"
#include "rx/core/abort.h"
#include "rx/core/global.h"

namespace rex {
    static Rx::Global<StdoutStream> stdout_stream{"system", "stdout_stream"};

#if TRACY_ENABLE
    void SetThreadName(void* /*_context*/, const char* _name) { tracy::SetThreadName(_name); }

    void BeginSample(void* /*_context*/, const Rx::Profiler::Sample* _sample) {
        const auto& source_info = _sample->source_location();

        // Enframe tracy::SourceLocationData in the Sample.
        auto enframe = _sample->enframe<tracy::SourceLocationData>();
        enframe->name = _sample->tag();
        enframe->function = source_info.function();
        enframe->file = source_info.file();
        enframe->line = source_info.line();

        TracyLfqPrepare(tracy::QueueType::ZoneBegin);
        tracy::MemWrite(&item->zoneBegin.time, tracy::Profiler::GetTime());
        tracy::MemWrite(&item->zoneBegin.srcloc, (uint64_t) enframe);
        TracyLfqCommit;
    }

    void EndSample(void* /*_context*/, const Rx::Profiler::Sample* /*_sample*/) {
        TracyLfqPrepare(tracy::QueueType::ZoneEnd);
        tracy::MemWrite(&item->zoneEnd.time, tracy::Profiler::GetTime());
        TracyLfqCommit;
    }
#endif

    Wrapper::Wrapper() {
        Rx::Globals::link();

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
    void to_json(json& j, const String& entry) { j = entry.data(); }

    void from_json(const json& j, String& entry) { entry = j.get<std::string>().c_str(); }
} // namespace Rx
