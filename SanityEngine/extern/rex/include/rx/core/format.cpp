#include <stdio.h> // vsnprintf

#include "rx/core/format.h"
#include "rx/core/assert.h"

namespace Rx {

Size format_buffer_va_list(char* buffer_, Size _length, const char* _format, va_list _list) {
  const int result = vsnprintf(buffer_, _length, _format, _list);
  RX_ASSERT(result > 0, "encoding error");
  return static_cast<Size>(result);
}

Size format_buffer_va_args(char* buffer_, Size _length, const char* _format, ...) {
  va_list va;
  va_start(va, _format);
  const Size result = format_buffer_va_list(buffer_, _length, _format, va);
  va_end(va);
  return result;
}

} // namespace Rx
