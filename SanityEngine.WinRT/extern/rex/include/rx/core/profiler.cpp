#include "rx/core/profiler.h"

namespace Rx {

Global<Profiler> Profiler::s_instance{"system", "profiler"};

} // namespace rx
