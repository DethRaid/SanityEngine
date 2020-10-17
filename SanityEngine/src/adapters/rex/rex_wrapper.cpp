#include "rex_wrapper.hpp"

#if TRACY_ENABLE
#include "Tracy.hpp"
#include "client/tracy_concurrentqueue.h"
#include "rx/core/profiler.h"
#endif

#include "adapters/rex/stdout_stream.hpp"
#include "rx/core/global.h"

using namespace tracy;

namespace rex {
    static Rx::Global<StdoutStream> stdout_stream{"system", "stdout_stream"};

#if TRACY_ENABLE
    void SetThreadName(void* /*_context*/, const char* _name) { tracy::SetThreadName(_name); }

    void BeginSample(void* /*_context*/, const Rx::Profiler::Sample* _sample) {
        const auto& source_info = _sample->source_location();
        
        // Enframe tracy::SourceLocationData in the Sample.
        auto enframe      = _sample->enframe<SourceLocationData>();
        enframe->name     = _sample->tag();
        enframe->function = source_info.function();
        enframe->file     = source_info.file();
        enframe->line     = source_info.line();
        
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
        Rx::Globals::link();

#if TRACY_ENABLE
        Rx::Profiler::instance().bind_cpu({nullptr, rex::SetThreadName, rex::BeginSample, rex::EndSample});
#endif
    }

    Wrapper::~Wrapper() {
#if TRACY_ENABLE
        Rx::Profiler::instance().unbind_cpu();
#endif

        Rx::Globals::fini();
    }
} // namespace rex
