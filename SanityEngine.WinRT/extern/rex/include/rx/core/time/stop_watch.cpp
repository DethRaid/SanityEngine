#include "rx/core/time/stop_watch.h"
#include "rx/core/time/qpc.h"

namespace Rx::Time {

void StopWatch::start() {
  m_start_ticks = qpc_ticks();
}

void StopWatch::stop() {
  m_stop_ticks = qpc_ticks();
}

Span StopWatch::elapsed() const {
  return {m_stop_ticks - m_start_ticks, qpc_frequency()};
}

} // namespace rx::time
