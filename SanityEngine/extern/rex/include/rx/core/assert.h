#ifndef RX_CORE_ASSERT_H
#define RX_CORE_ASSERT_H
#include "rx/core/source_location.h"
#include "rx/core/format.h"

namespace Rx {

[[noreturn]] RX_API void assert_message(const char* _expression,
  const SourceLocation& _source_location, const char* _message, bool _truncated);

template<typename... Ts>
[[noreturn]]
void assert_fail(const char* _expression,
  const SourceLocation& _source_location, const char* _format,
  Ts&&... _arguments)
{
  // When we have format arguments use an on-stack format buffer.
  if constexpr(sizeof...(Ts) > 0) {
    char buffer[4096];
    const Size length = format_buffer(buffer, sizeof buffer, _format,
      Utility::forward<Ts>(_arguments)...);
    assert_message(_expression, _source_location, buffer, length >= sizeof buffer);
  } else {
    assert_message(_expression, _source_location, _format, false);
  }
}

} // namespace rx

#if defined(RX_DEBUG)
#define RX_ASSERT(_condition, ...) \
  (static_cast<void>((_condition) \
    || (::Rx::assert_fail(#_condition, RX_SOURCE_LOCATION, __VA_ARGS__), 0)))
#else // defined(RX_DEBUG)
#define RX_ASSERT(_condition, ...) \
  static_cast<void>((_condition) || 0)
#endif // defined(RX_DEBUG)

#endif // RX_CORE_ASSERT_H
