#include <string.h> // memcpy

#include "rx/core/vector.h"

namespace Rx::detail {

void copy(void *RX_HINT_RESTRICT dst_, const void* RX_HINT_RESTRICT _src, Size _size) {
  // Help check for undefined calls to memcpy.
  RX_ASSERT(dst_, "null destination");
  RX_ASSERT(_src, "null source");
  RX_ASSERT(_size, "zero byte copy");

  memcpy(dst_, _src, _size);
}

} // namespace rx
