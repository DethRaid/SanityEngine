#ifndef RX_CORE_TIME_SPAN_H
#define RX_CORE_TIME_SPAN_H
#include "rx/core/format.h"

namespace Rx::Time {

struct RX_API Span {
  constexpr Span(Uint64 _start_ticks, Uint64 _stop_ticks, Uint64 _frequency);
  constexpr Span(Uint64 _ticks, Uint64 _frequency);
  constexpr Span(Sint64 _ticks, Uint64 _frequency);

  // The number of days, hours, minutes & milliseconds of this span.
  Sint64 days() const;
  Sint64 hours() const;
  Sint64 minutes() const;
  Sint64 seconds() const;
  Float64 milliseconds() const;

  Float64 total_days() const;
  Float64 total_hours() const;
  Float64 total_minutes() const;
  Float64 total_seconds() const;
  Float64 total_milliseconds() const;

private:
  constexpr Span(bool _negate, Uint64 _ticks, Uint64 _frequency);

  // Store sign separately from |m_ticks| for representing ranges that go in
  // the opposite direction. This increases our effective time span range by
  // a factor of two.
  //
  // This also acts as a useful Type-conversion since all calculations are
  // done on unsigned integers, the multiplication by a signed sign makes
  // everything signed, avoiding the need for explicit casts.
  Sint64 m_sign;
  Uint64 m_ticks;     // Number of ticks.
  Uint64 m_frequency; // Number of ticks per second.
};

constexpr Span::Span(Uint64 _start_ticks, Uint64 _stop_ticks, Uint64 _frequency)
  : Span{_start_ticks < _stop_ticks ? _stop_ticks - _start_ticks : _start_ticks - _stop_ticks, _frequency}
{
}

constexpr Span::Span(Uint64 _ticks, Uint64 _frequency)
  : Span{false, _ticks, _frequency}
{
}

constexpr Span::Span(Sint64 _ticks, Uint64 _frequency)
  : Span{_ticks < 0, static_cast<Uint64>(_ticks < 0 ? -_ticks : _ticks), _frequency}
{
}

constexpr Span::Span(bool _negate, Uint64 _ticks, Uint64 _frequency)
  : m_sign{_negate ? -1 : 1}
  , m_ticks{_ticks}
  , m_frequency{_frequency}
{
}

} // namespace rx::time

namespace Rx {
  template<>
  struct FormatNormalize<Time::Span> {
    // Largest possible string is:
    // "%dd:%02dh:%02dm:%02ds:%.2fms"
    char scratch[FormatSize<Sint64>::size + sizeof ":  :  :  :    ms"];
    const char* operator()(const Time::Span& _value);
  };
}

#endif // RX_CORE_TIME_SPAN_H
