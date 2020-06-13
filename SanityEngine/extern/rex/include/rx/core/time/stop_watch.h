#ifndef RX_CORE_TIME_STOP_WATCH_H
#define RX_CORE_TIME_STOP_WATCH_H
#include "rx/core/time/span.h"

namespace Rx::Time {

struct StopWatch {
  constexpr StopWatch();

  void reset();
  void restart();
  void start();
  void stop();

  bool is_running() const;

  Span elapsed() const;

private:
  Uint64 m_start_ticks;
  Uint64 m_stop_ticks;
};

constexpr StopWatch::StopWatch()
  : m_start_ticks{0}
  , m_stop_ticks{0}
{
}

inline void StopWatch::reset() {
  m_start_ticks = 0;
  m_stop_ticks = 0;
}

inline void StopWatch::restart() {
  reset();
  start();
}

inline bool StopWatch::is_running() const {
  return m_start_ticks != 0;
}

} // namespace rx::time

#endif // RX_CORE_TIME_STOP_WATCH_H
