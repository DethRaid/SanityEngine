#include <inttypes.h> // PRId64
#include <stdio.h> // snprintf

#include "rx/core/time/span.h"
#include "rx/core/math/mod.h"
#include "rx/core/hints/force_inline.h"

namespace Rx::Time {

RX_HINT_FORCE_INLINE Uint64 ticks_per_second(Uint64 _frequency) {
  return _frequency;
}

RX_HINT_FORCE_INLINE Uint64 ticks_per_minute(Uint64 _frequency) {
  return ticks_per_second(_frequency) * 60;
}

RX_HINT_FORCE_INLINE Uint64 ticks_per_hour(Uint64 _frequency) {
  return ticks_per_minute(_frequency) * 60;
}

RX_HINT_FORCE_INLINE Uint64 ticks_per_day(Uint64 _frequency) {
  return ticks_per_hour(_frequency) * 24;
}

Sint64 Span::days() const {
  const auto scale = ticks_per_day(m_frequency);
  return (m_ticks / scale) * m_sign;
}

Sint64 Span::hours() const {
  const auto scale = ticks_per_hour(m_frequency);
  return ((m_ticks / scale) % 24) * m_sign;
}

Sint64 Span::minutes() const {
  const auto scale = ticks_per_minute(m_frequency);
  return ((m_ticks / scale) % 60) * m_sign;
}

Sint64 Span::seconds() const {
  const auto scale = ticks_per_second(m_frequency);
  return ((m_ticks / scale) % 60) * m_sign;
}

Float64 Span::milliseconds() const {
  // NOTE(dweiler): Scale |m_ticks| towards seconds then divide by frequency
  // which is always given as "ticks per second", so that the division by scale
  // later is in units of the frequency, this has better rounding behavior for
  // when the frequency doesn't divide evenly into 1000, since the floor is
  // moved to the end.
  //
  // This behavior is also used in |total_milliseconds| for the same reason.
  const auto scale = ticks_per_second(m_frequency);
  const auto result = (m_ticks * 1000.0) / scale;
  const auto sign = m_sign ? 1.0 : -1.0;
  return (result > 1000.0 ? Math::mod(result, 1000.0) : result) * sign;
}

Float64 Span::total_days() const {
  const auto scale = 1.0 / ticks_per_day(m_frequency);
  return m_ticks * scale;
}

Float64 Span::total_hours() const {
  const auto scale = 1.0 / ticks_per_hour(m_frequency);
  return m_ticks * scale;
}

Float64 Span::total_minutes() const {
  const auto scale = 1.0 / ticks_per_minute(m_frequency);
  return m_ticks * scale;
}

Float64 Span::total_seconds() const {
  const auto scale = 1.0 / ticks_per_second(m_frequency);
  return m_ticks * scale;
}

Float64 Span::total_milliseconds() const {
  // Use the same behavior here as |milliseconds|.
  //
  // NOTE(dweiler): Would be dangerous to introduce something like
  // |ticks_per_milliseconds| as the |scale| would require tiny, nearly denormal
  // Float64 to express the reciprocal of large |m_frequency|, so keeping things
  // in units of seconds avoids catastropic precision loss for high precision
  // timers.
  const auto scale = 1.0 / ticks_per_second(m_frequency);
  return (m_ticks * 1000.0) * scale;
}

} // namespace rx::time

const char* Rx::FormatNormalize<Rx::Time::Span>::operator()(const Time::Span& _value) {
  const auto days = _value.days();
  const auto hours = _value.hours();
  const auto minutes = _value.minutes();
  const auto seconds = _value.seconds();
  const auto milliseconds = _value.milliseconds();

  if (days) {
    snprintf(scratch, sizeof scratch,
      "%" PRId64 "d:%02" PRId64 "h:%02" PRId64 "s:%02" PRId64 "m:%.2fms",
      days, hours, minutes, seconds, milliseconds);
  } else if (hours) {
    snprintf(scratch, sizeof scratch,
      "%02" PRId64 "h:%02" PRId64 "m:%02" PRId64 "s:%.2fms",
      hours, minutes, seconds, milliseconds);
  } else if (minutes) {
    snprintf(scratch, sizeof scratch,
      "%02" PRId64 "m:%02" PRId64 "s:%.2fms",
      minutes, seconds, milliseconds);
  } else if (seconds) {
    snprintf(scratch, sizeof scratch,
      "%02" PRId64 "s:%.2fms",
      seconds, milliseconds);
  } else {
    snprintf(scratch, sizeof scratch,
      "%.2fms",
      milliseconds);
  }

  return scratch;
}
