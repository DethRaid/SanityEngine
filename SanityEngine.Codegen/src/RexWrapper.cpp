#include "RexWrapper.hpp"

#include <cstdio>

#include "Rex/StdoutStream.hpp"
#include "nlohmann/json.hpp"
#include "rx/core/abort.h"
#include "rx/core/global.h"
#include "rx/core/log.h"
#include "rx/core/profiler.h"
#include "rx/core/string.h"

namespace rex {
    static Rx::Global<StdoutStream> stdout_stream{"system", "stdout_stream"};

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

namespace Rx {
    void to_json(json& j, const Rx::String& entry) { j = json{entry.data()}; }

    void from_json(const json& j, Rx::String& entry) { entry = j.get<std::string>().c_str(); }
} // namespace Rx
