#include <string.h> // memcpy

#include "rx/core/vector.h"

namespace Rx::detail {

void copy(void *RX_HINT_RESTRICT dst_, const void* RX_HINT_RESTRICT _src, Size _size) {
  memcpy(dst_, _src, _size);
}

} // namespace rx
