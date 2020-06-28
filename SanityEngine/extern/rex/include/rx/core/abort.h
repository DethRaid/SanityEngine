#ifndef RX_CORE_ABORT_H
#define RX_CORE_ABORT_H
#include "rx/core/format.h"

namespace Rx {

[[noreturn]]
void abort_message(const char* _message, bool _truncated);

template<typename... Ts>
[[noreturn]]
void abort(const char* _format, Ts&&... _arguments) {
  // When we have format arguments use an on-stack format buffer.
  if constexpr(sizeof...(Ts) > 0) {
    char buffer[4096];
    const Size length = format_buffer(buffer, sizeof buffer, _format,
      Utility::forward<Ts>(_arguments)...);
    abort_message(buffer, length >= sizeof buffer);
  } else {
    abort_message(_format, false);
  }
}

} // namespace rx

#endif // RX_CORE_ABORT_H
