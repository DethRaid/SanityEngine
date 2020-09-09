#include "rex_wrapper.hpp"

#include <cstdio>

#if TRACY_ENABLE
#include "Tracy.hpp"
#endif

#include "rx/core/abort.h"
#include "rx/core/global.h"
#include "rx/core/log.h"
#include "rx/core/profiler.h"
#include "adapters/rex/stdout_stream.hpp"

namespace rex {
    static Rx::Global<StdoutStream> stdout_stream{"system", "stdout_stream"};

#if TRACY_ENABLE
    void SetThreadName(void* /*_context*/, const char* _name) { tracy::SetThreadName(_name); }

    void BeginSample(void* /*_context*/, const Rx::Profiler::Sample* _sample) {
        // const auto& source_info = _sample->source_location();
        //
        // // Enframe tracy::SourceLocationData in the Sample.
        // auto enframe = _sample->enframe<tracy::SourceLocationData>();
        // enframe->name = _sample->tag();
        // enframe->function = source_info.function();
        // enframe->file = source_info.file();
        // enframe->line = source_info.line();
        //
        // TracyLfqPrepare(tracy::QueueType::ZoneBegin);
        // tracy::MemWrite(&item->zoneBegin.time, tracy::Profiler::GetTime());
        // tracy::MemWrite(&item->zoneBegin.srcloc, (uint64_t) enframe);
        // TracyLfqCommit;
    }

    void EndSample(void* /*_context*/, const Rx::Profiler::Sample* /*_sample*/) {
        // TracyLfqPrepare(tracy::QueueType::ZoneEnd);
        // tracy::MemWrite(&item->zoneEnd.time, tracy::Profiler::GetTime());
        // TracyLfqCommit;
    }
#endif

    Wrapper::Wrapper() {
        if(initialized) {
            Rx::abort("Rex is already initialized");
        }

#if TRACY_ENABLE
        Rx::Profiler::instance().bind_cpu({nullptr, SetThreadName, BeginSample, EndSample});
#endif

        Rx::Globals::link();

        Rx::GlobalGroup* system_group{Rx::Globals::find("system")};

        // Explicitly initialize globals that need to be initialized in a specific
        // order for things to work.
        system_group->find("heap_allocator")->init();
        system_group->find("allocator")->init();
        stdout_stream.init();
        system_group->find("logger")->init();

        const auto subscribed = Rx::Log::subscribe(&stdout_stream);
        if(!subscribed) {
            fprintf(stderr, "Could not attach stdout stream to logger");
        }

        Rx::Globals::init();

        initialized = true;
    }

    Wrapper::~Wrapper() {
        if(!initialized) {
            Rx::abort("You're trying to deinit Rex without first initting it, not sure how you did this but please stop");
        }

        Rx::GlobalGroup* system_group{Rx::Globals::find("system")};

        Rx::Globals::fini();

        system_group->find("logger")->fini();
        stdout_stream.fini();
        system_group->find("allocator")->fini();
        system_group->find("heap_allocator")->fini();

#if TRACY_ENABLE
        Rx::Profiler::instance().unbind_cpu();
#endif
    }
} // namespace rex
